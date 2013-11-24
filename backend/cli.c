#include "../include/cli.h"

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
    size_t msg_len;
    int r = 0;

    f->pack_id = con->pack_id = rand() % MAX_PACK_NUM;
    con->status = SRV_READY;
    con->piece_id = f->piece_id;

    msg_len = decode_cli_msg(f, msg);
    if (msg_len <= 0) {
        err(CLIENT, "decode message error");
        r = TRME_DECODE_ERR;
    } else {
        log_cli_fields(f);
        if (send_data(con->srv_sock, msg, msg_len, 0) < 0) {
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
    if (con->status != SRV_READY && con->timeout-- <= 0) {
        log(CLIENT, "timeout on %d socket", con->srv_sock);
        r = 1;
        con->status = SRV_READY;
        q = &((t_descr.trm + con->transmission_id)->pieces);
        if (con->piece_id == q->cur_piece - 1)
            q->cur_piece--;
        else if (q->max_failed_piece_num >= MAX_PIECES_COUNT) {
            if (q->cur_piece > con->piece_id)
                q->cur_piece = con->piece_id;
        } else
            q->failed_pieces[q->max_failed_piece_num++] = q->cur_piece;
    } else if (con->status == SRV_READY)
        r = 1;
    return r;
}

/**
 * Ф-ия запрашивает очередной кусок.
 * Возвращает указатель на следующий элемент для обработки
 */
static void request_piece(struct active_connection *con) {
    struct pieces_queue *q;
    struct transmission *t;
    struct cli_fields f;
    piece_id_t piece;
    int i, j;
    int pirate = 0;

    if (con->status == SRV_READY) {
        log(CLIENT, "server %d is ready", con->srv_sock);
        t = t_descr.trm + con->transmission_id;
        q = &(t->pieces);
        if (q->max_failed_piece_num >= 0) {
            piece = q->failed_pieces[0];
            j = 0;
            for (i = 1; i < q->max_failed_piece_num; i++)
                if (q->failed_pieces[i] < piece) {
                    piece = q->failed_pieces[i];
                    j = i;
                }
            for (i = j; i < q->max_failed_piece_num; i++)
                q->failed_pieces[i] = q->failed_pieces[i + 1];
            q->max_failed_piece_num--;
        } else if (q->cur_piece <= q->max_piece_num)
            piece = q->cur_piece++;
        else
            pirate = 1;

        if (!pirate) {
            log(CLIENT, "require piece with id %d", piece);
            f.piece_id = piece;
            f.file_id = con->file_id;
            f.error = 0;
            /* TODO */
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
        for (r = 0; !t->openned_trms[r] && r < st_arr_len(t->trm); r++);
        log(CLIENT, "trm: %d", r);
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
        q->max_piece_num = file_size / DATA_BLOCK_LEN;
        q->flushed_pieces_count = 0;
        if (file_size != q->max_piece_num * DATA_BLOCK_LEN)
            q->max_piece_num++;

        log(CLIENT, "max_piece_num: %d", q->max_piece_num);
    }
    return r;
}

/**
 * Ф-ия меняет указатели для q и освобождает из под него память
 * Возвращает следующий по списку элемент
 */
inline static struct active_connection *free_con_ptr(struct active_connection *q) {
    struct active_connection *t = q->next;
    if (q->next)
        q->next->prev = q->prev;
    else
        q_descr.q_tail = q->prev;
    if (q->prev)
        q->prev->next = q->next;
    else
        q_descr.q_head = q->next;
    free(q);
    return t;
}

/**
 * Ф-ия удаляет соединение (на случай ошибки на стороне сервера) из
 * списка
 */
static void remove_connection(struct active_connection *c) {
    int f = 0;
    struct active_connection *q = q_descr.q_head;

    while (!f && q) {
        if (q != c && q->transmission_id == c->transmission_id)
            f = 1;
        q = q->next;
    }
    if (!f) {
        t_descr.count--;
        fclose(t_descr.trm[c->transmission_id].file);
        t_descr.openned_trms[c->transmission_id] = 1;
        free(c->data);
    }
    free_con_ptr(c);
}

/**
 * Удаляет из списка активных передач передачу номер tr_id
 */
static void remove_transmission(int tr_id) {
    struct active_connection *q = q_descr.q_head;
    int pirate = 1;

    t_descr.count--;
    if (t_descr.trm[tr_id].file) {
        fclose(t_descr.trm[tr_id].file);
        t_descr.trm[tr_id].file = NULL;
    }
    t_descr.openned_trms[tr_id] = 1;

    while (q) {
        if (q->transmission_id == tr_id) {
            if (pirate) {
                free(q->data);
                pirate = 0;
            }
            q = free_con_ptr(q);
        }
    }
}

/**
 * Ф-ия добавляет соединение с сервером для передачи файла
 */
static struct active_connection *add_connection(int sock,
        int transmission_id, piece_id_t piece_id) {
    struct active_connection *q;

    q = m_alloc_s(struct active_connection);
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
    struct file_full_data_t *data;
    struct transmission *t = t_descr.trm + transmission_id;
    struct cli_fields f = { .error = 0, .file_id = -1 };

    data = (struct file_full_data_t *)calloc(sizeof(struct file_full_data_t), 1);
    if (!data) {
        err_n(CLIENT, "transmission start failure");
        r = TRME_ALLOC_FAILURE;
    } else {
        strncpy(f.file_name, t->filename, FILE_NAME_MAX_LEN);
        snprintf(fname, st_arr_len(fname), "%s/%s", APP_DIR_PATH, t->filename);
        memcpy(f.hsumm, t->filesum, MD5_DIGEST_LENGTH);

        data->data.f_piece = sizeof(data->data.data) / DATA_BLOCK_LEN;

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
            con->piece_id == f->piece_id)
            r = con;
        con = con->next;
    }
    return r;
}

/**
 * Ф-ия добавляет новые данные к уже полученным
 */
static void push_file_data(struct file_full_data_t *data,
        const struct srv_fields *f, const struct transmission *t) {
    const struct cli_fields *cli = &(f->cli_field);
    struct file_udata_t *ud;
    struct file_data_t *d;
    unsigned char *d_ptr = NULL;
    size_t piece_len = f->piece_len;
    int i;

    if (cli->piece_id == t->pieces.max_piece_num - 1)
        piece_len = t->filesize - (cli->piece_id * DATA_BLOCK_LEN);

    if (cli->piece_id >= data->data.f_piece) {
        /* Откладываем данные до лучших времен */
        ud = data->udata;
        for (i = 0; i < st_arr_len(data->udata); i++) {
            if (ud->piece_len == 0) {
                i = st_arr_len(data->udata);
                ud->piece_id = cli->piece_id;
                ud->piece_len = piece_len;
                d_ptr = ud->data;
            }
            ud++;
        }
    } else {
        /* Копируем блок в общий буфер */
        d = &(data->data);
        d->full_size += piece_len;
        d->pieces_copied++;
        d_ptr = d->data + (cli->piece_id - d->s_piece) * DATA_BLOCK_LEN;
    }
    if (d_ptr)
        memcpy(d_ptr, f->piece, f->piece_len);
    else
        err(CLIENT, "Piece %d saving failure", cli->piece_id);
}

/**
 * Ф-ия сбрасывает полученную часть файла на диск
 * Возвращает 1 если передача была завершена
 */
static int flush_file_data(struct file_full_data_t *data, FILE *file,
        piece_id_t max_piece_num, struct transmission *t) {
    struct file_data_t *d = &(data->data);
    struct file_udata_t *ud;
    FILE *ffff;
    char a[20], b[20];
    int i, r = 0, st = 0;

    if (d->pieces_copied == d->f_piece - d->s_piece) {
        /* Данные можно сбрасывать на жесткий диск */
        log(CLIENT, "writing %lu bytes", d->full_size);
        ffff = fopen("/tmp/course_prj/results", "a");
        memcpy(a, d->data, 20);
        memcpy(b, d->data + d->full_size - 20, 20);
        a[19] = 0;
        b[19] = 0;
        fprintf(ffff, "Writing %lu bytes: start piece id: %d, end piece id: %d, start 10 bytes: %s, "
                "end 10 bytes: %s\n", d->full_size, d->s_piece, d->f_piece, a, b);
        fclose(ffff);
        if (fwrite(d->data, sizeof(d->data[0]), d->full_size, file) != d->full_size)
            err_n(CLIENT, "fwrite failure");

        d->s_piece = d->f_piece;
        t->pieces.flushed_pieces_count += d->pieces_copied;
        d->pieces_copied = 0;
        d->full_size = 0;
        d->f_piece += st_arr_len(data->data.data) / DATA_BLOCK_LEN;
        if (d->f_piece > max_piece_num)
            d->f_piece = max_piece_num;

        ud = data->udata;
        for (i = 0; i < st_arr_len(data->udata); i++) {
            if (ud->piece_len != 0 && ud->piece_id < d->f_piece) {
                st = 1;
                d->pieces_copied++;
                d->full_size += ud->piece_len;
                memcpy(d->data + (ud->piece_id - d->s_piece) * DATA_BLOCK_LEN, ud->data, ud->piece_len);
                ud->piece_len = 0;
            }
            ud++;
        }
    }

    if (!st && t->pieces.max_piece_num == t->pieces.flushed_pieces_count) {
        log(CLIENT, "receive successfully completed");
        r = 1;
    }
    return r;
}

/**
 * Ф-ия помечает полученный кусок как полученный и добавляет его
 * в результирующие данные
 */
static void process_received_piece(const struct srv_fields *f,
        struct active_connection *con) {
    struct transmission *t = t_descr.trm + con->transmission_id;
    struct pieces_queue *p;
    char err_msg[255];

    con->status = SRV_READY;
    if (f->cli_field.file_id < 0) {
        decode_proto_error(f->cli_field.error, err_msg, st_arr_len(err_msg));
        err(CLIENT, "transmission failure: %s", err_msg);
        p = &(t->pieces);
        p->failed_pieces[++(p->max_failed_piece_num)] = f->cli_field.piece_id;
        remove_connection(con);
    } else {
        con->file_id = f->cli_field.file_id;
        push_file_data(con->data, f, t);
        if (flush_file_data(con->data, t->file, t->pieces.max_piece_num, t))
            remove_transmission(con->transmission_id);
    }
}

/**
 * Запускает для передачи файл с определенной хэш-суммой.
 * Возвращает номер открытой передачи.
 * В случае ошибки возвращает отрицательное число, которое можно
 * интерпретировать с помощью TRME_* флагов
 */
int receive_file(const char *filename, const unsigned char *hsum,
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
        if (process_timeouts(cur))
            request_piece(cur);
        cur = cur->next;
    }
}

/**
 * Callback-функция которая вызывается при получении сообщения клиентом.
 * Обрабатывает сообщение полученное от сервера.
 */
int process_srv_message(int sock, const char *msg, size_t len) {
    static struct srv_fields fields;    /**< Чтобы лишний раз не выделять память в стеке */
    struct active_connection *con;
    int r = 0;

    if ((r = encode_srv_msg(&fields, msg, len)) == 0) {
        log_srv_fields(&fields);
        if ((con = search_connection(sock, &(fields.cli_field))))
            process_received_piece(&fields, con);
        else
            r = -1;
    }
    return r;
}
