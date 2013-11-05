#include "cli.h"

static struct active_connections q_descr = {
    .q_head = NULL,
    .q_tail = NULL
};
static struct transmissions t_descr = {
    .count = 0,
    .openned_trms = { [0 ... (MAX_TRANSMISSIONS - 1)] = 1 },
};


/**
 * Функция отправляет запрос на получение куска файла
 * В случае ошибки, ф-ия возвращает ее код который можно
 * интерпретировать с помощью TRME_* флагов,
 * иначе -- 0
 */
static int require_piece(struct cli_fields *f, struct active_connection *con) {
    char msg[BUF_MAX_LEN];
    ssize_t msg_len;
    int r = 0;

    f->pack_id = con->pack_id = rand() % MAX_PACK_NUM;
    con->status = SRV_READY;
    con->piece_id = f->piece_id;

    msg_len = decode_cli_msg(f, msg);
    if (msg_len <= 0) {
        err(CLIENT, "decode message error");
        r = TRME_DECODE_ERR;
    } else {
        if (send(con->srv_sock, msg, msg_len, 0) < 0) {
            err_n(CLIENT, "send failure");
            r = TRME_SOCKET_FAILURE;
            con->status = SRV_UNKN;
        } else {
            con->status = SRV_BUSY;
            con->timeout = FILE_TIMEOUT;
        }
    }
    return r;
}

/**
 * Ф-ия обрабатывает переданный ей элемент на предмет истечения
 * времени ожидания.
 */
static int process_timeouts(struct active_connection *con) {
    struct pieces_queue *q;
    int r = 0;
    if (con->timeout-- <= 0) {
        r = 1;
        con->status = TRM_UNKN;
        q = &((t_descr.trm + con->transmission_id)->pieces);
        if (con->piece_id == q->cur_piece - 1)
            q->cur_piece--;
        else if (q->max_failed_piece_num >= MAX_PIECES_COUNT) {
            if (q->cur_piece > con->piece_id)
                q->cur_piece = con->piece_id;
        } else
            q->failed_pieces[q->max_failed_piece_num++] = q->cur_piece;
    }
    return r;
}

/**
 * Ф-ия запрашивает очередной кусок
 */
static void request_piece(struct active_connection *con) {
    struct pieces_queue *q;
    struct transmission *t;
    struct cli_fields f;
    piece_id_t piece;

    if (con->status == SRV_READY) {
        t = t_descr.trm + con->transmission_id;
        q = &(t->pieces);
        if (q->max_failed_piece_num >= 0)
            piece = q->failed_pieces[--q->max_failed_piece_num];
        else
            piece = q->cur_piece++;

        if (piece > q->max_piece_num) {
            /* Файл полностью передан */
            if (con->next)
                con->next->prev = con->prev;
            else
                q_descr.q_tail = con->prev;
            if (con->prev)
                con->prev->next = con->next;
            else
                q_descr.q_head = con->next;
            free(con);
        } else {
            f.piece_id = piece;
            strncpy(f.file_name, t->filename, FILE_NAME_MAX_LEN);
            memcpy(f.hsumm, t->filesum, MD5_DIGEST_LENGTH);

            /* TODO: Обработать ошибки */
            require_piece(&f, con);
        }
    }
}

/**
 * Функция добавляет активную передачу в список.
 * Возвращает номер добавленной передачи.
 * В случае ошибки возвращает код ошибки (< 0)
 */
static int add_transmission(const char *filename,
        const unsigned char *filesum, unsigned long file_size) {
    struct transmissions *t = &t_descr;
    struct transmission *trm;
    struct pieces_queue *q;
    int r = 0;

    if (t->count >= MAX_TRANSMISSIONS) {
        err(CLIENT, "add transmission failure: too many transmissions");
        r = TRME_TOO_MANY_TRM;
    }
    if (!r) {
        for (r = 0; !t->openned_trms[r] && r < sizeof(t->trm); r++);
        t->openned_trms[r] = 0;
        t->count++;
        trm = t->trm + r;
        strncpy(trm->filename, filename, FILE_NAME_MAX_LEN);
        memcpy(trm->filesum, filesum, MD5_DIGEST_LENGTH);
        trm->filesize = file_size;
        trm->status = TRM_WAITING_SERVERS;

        q = &trm->pieces;
        q->max_failed_piece_num = -1;
        q->cur_piece = 0;
        q->max_piece_num = file_size / BUF_MAX_LEN;
        if (file_size != q->max_piece_num * BUF_MAX_LEN)
            q->max_piece_num++;
    }
    return r;
}

/**
 * Удаляет из списка активных передач передачу номер tr_id
 */
static void remove_transmission(int tr_id) {
    t_descr.count--;
    t_descr.openned_trms[tr_id] = 1;
}

/**
 * Ф-ия добавляет соединение с сервером для передачи файла
 */
static struct active_connection *add_connection(int sock,
        int transmission_id, piece_id_t piece_id) {
    struct active_connection *q;

    q = m_alloc_s(struct active_connection *);
    q->srv_sock = sock;
    q->timeout = FILE_TIMEOUT;
    q->transmission_id = transmission_id;
    q->status = SRV_BUSY;
    q->piece_id = piece_id;
    q->next = NULL;
    q->prev = NULL;
    if (!q_descr.q_head)
        q_descr.q_head = q;
    if (q_descr.q_tail) {
        q_descr.q_tail->next = q;
        q->prev = q_descr.q_tail;
    }
    q_descr.q_tail = q;
    return q;
}

/**
 * Функция рассылает всем активным серверам запрос на
 * получение файла с заранее определенной хэш-суммой.
 * В случае ошибки функция возвращает ненулевое значение,
 * которое можно интерпретировать с помощью TRME_* флагов
 */
static int start_transmission(int transmission_id,
        const struct sockets_queue *q) {
    int i, req_count;
    int r = 0;
    char fname[FILE_NAME_MAX_LEN];
    struct active_connection *con;
    struct file_data_t *data, *tmp;
    struct transmission *t = t_descr.trm + transmission_id;
    struct cli_fields f = { .error = 0 };
    int elem_count;

    elem_count = t->pieces.max_piece_num;
    if (elem_count > MIN_ALLOCATED_PIECES)
        elem_count = MIN_ALLOCATED_PIECES;

    data = m_alloc(struct file_data_t *, elem_count);
    if (!data) {
        err_n(CLIENT, "transmission start failure");
        r = TRME_ALLOC_FAILURE;
    } else {
        tmp = data;
        for (i = 0; i < elem_count; i++) {
            tmp->next = (i == elem_count - 1 ? NULL : tmp + 1);
            tmp->prev = (i == 0 ? NULL : tmp - 1);
            tmp++;
        }

        strncpy(f.file_name, t->filename, FILE_NAME_MAX_LEN);
        snprintf(fname, sizeof(fname), "%s/%s", APP_DIR_PATH, t->filename);
        memcpy(f.hsumm, t->filesum, MD5_DIGEST_LENGTH);

        t->file = fopen(fname, "wb");
        if (!t->file) {
            err_n(CLIENT, "fopen failure");
            r = TRME_FILE_ERROR;
        } else if (ftruncate(fileno(t->file), t->filesize)) {
            err_n(CLIENT, "ftruncate failure");
            r = TRME_OUT_OF_MEMORY;
        }

        for (i = 0, req_count = 0; !r && i < q->count && i < t->pieces.max_piece_num; i++) {
            con = add_connection(q->sockets[i], transmission_id, i);
            con->data = data;
            if (con) {
                f.piece_id = i;
                /* TODO: Обработать исключительные ситуации */
                if (require_piece(&f, con) == 0) {
                    req_count++;
                    t->pieces.cur_piece++;
                }
            } else
                r = TRME_ALLOC_FAILURE;
        }
        if (!r && !req_count) {
            r = TRME_NO_ACTIVE_SRVS;
            err(CLIENT, "no active servers found to start transmission");
        }
    }
    return r;
}

/**
 * Ф-ия ищет в списке активных соединений подходящее и возвращает его.
 * Если соединение не найдено, возвращает 0.
 */
static struct active_connection *search_connection(int sock, const struct cli_fields *f) {
    struct active_connection *con, *r = NULL;
    con = q_descr.q_head;
    while (!r && con) {
        if (con->srv_sock == sock &&
            con->pack_id == f->pack_id &&
            con->file_id == f->file_id &&
            con->piece_id == f->piece_id)
            r = con;
        con = con->next;
    }
    return r;
}

/**
 * Ф-ия добавляет новые данные к уже полученным
 */
static void push_file_data(struct file_data_t *data, const struct srv_fields *f) {
    /* TODO */
}

/**
 * Ф-ия уплотняет полученные данные
 */
static void compact_file_data(struct file_data_t *data) {
    /* TODO */
}

/**
 * Ф-ия сбрасывает полученную часть файла на диск
 */
static void flush_file_data(struct file_data_t *data, FILE *file) {
    /* TODO */
}

/**
 * Ф-ия помечает полученный кусок как полученный и добавляет его
 * в результирующие данные
 */
static void process_recieved_piece(const struct srv_fields *f,
        struct active_connection *con) {
    con->status = SRV_READY;
    push_file_data(con->data, f);
    compact_file_data(con->data);
    flush_file_data(con->data, (t_descr.trm + con->transmission_id)->file);
}

/**
 * Запускает для передачи файл с определенной хэш-суммой.
 * Возвращает номер открытой передачи.
 * В случае ошибки возвращает отрицательное число, которое можно
 * интерпретировать с помощью TRME_* флагов
 */
int recieve_file(const char *filename, const unsigned char *hsum,
        unsigned long fsize, const struct sockets_queue *q) {
    int tr_no, r;
    tr_no = add_transmission(filename, hsum, fsize);
    r = tr_no;
    if (tr_no >= 0) {
        r = start_transmission(tr_no, q);
        if (r) {
            remove_transmission(tr_no);
            r = -1;
        }
    }
    return r;
}

/*
 * Обрабатывает список активных передач.
 */
void main_dispatcher() {
    struct active_connection *cur;
    cur = q_descr.q_head;
    while (cur) {
        process_timeouts(cur);
        request_piece(cur);
    }
}

/**
 * Callback-функция которая вызывается при получении сообщения клиентом.
 * Обрабатывает сообщение полученное от сервера.
 */
int process_srv_message(int sock, const char *msg, ssize_t len) {
    static struct srv_fields fields;    /**< Слишком большая для стека */
    struct active_connection *con;
    int r = 0;

    log(CLIENT, "package recieved from server %d", sock);

    if ((r = encode_srv_msg(&fields, msg, len)) == 0) {
        if ((con = search_connection(sock, &(fields.cli_field))))
            process_recieved_piece(&fields, con);
        else
            r = -1;
    }
    return r;
}
