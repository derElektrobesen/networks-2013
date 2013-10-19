#include "cli.h"

#define m_alloc(type, count) ((type)malloc(sizeof(type) * (count)))
#define m_alloc_s(type) (m_alloc(type, 1)) /* Allocate single obj */

static struct active_queries q_descr = {
    .q_head = NULL,
    .q_tail = NULL
};
static struct transmissions t_descr = { .count = 0 };

static int process_timeouts() {
    return 0;
}

static int process_distrib() {
    return 0;
}

/* Функция рассылающая сообщения серверам в локальной сети */
static int response_servers(const struct sockets_queue *q) {
    return 0;
}

/**
 * Функция добавляет активную передачу в список.
 * Возвращает номер добавленной передачи.
 * В случае ошибки возвращает код ошибки (< 0)
 */
static int add_transmission(const char *filename, 
        const unsigned char *filesum) {
    struct transmissions *t = &t_descr;
    struct transmission *trm;
    int r = 0;
    
    if (t->count >= MAX_TRANSMISSIONS) {
        err(CLIENT, "add transmission failure: too many transmissions");
        r = TRME_TOO_MANY_TRM;
    }
    if (!r) {
        r = t->count++;
        trm = t->trm + r;
        strncpy(trm->filename, filename, FILE_NAME_MAX_LEN);
        memcpy(trm->filesum, filesum, MD5_DIGEST_LENGTH);
        trm->status = TRM_WAITING_SERVERS;
    }
    return r;
}

/**
 * Удаляет из списка активных передач передачу номер tr_id
 */
static void remove_transmission(int tr_id) {
    struct transmissions *t = &t_descr;
    int i;
    for (i = tr_id; i < t->count; i++)
        t->trm[i] = t->trm[i + 1];
    t->count--;
}

/**
 * Функция отправляет запрос на получение куска файла
 * В случае ошибки, ф-ия возвращает ее код который можно
 * интерпретировать с помощью TRME_* флагов,
 * иначе -- 0
 */
static int require_piece(struct cli_fields *f, int transmission_id,
        piece_num_t piece_id, int sock) {
    struct active_query *q;
    struct active_queries *queue = &q_descr;
    char msg[BUF_MAX_LEN];
    ssize_t msg_len;
    int r = 0;

    f->pack_id = rand() % MAX_PACK_NUM;
    f->piece_num = piece_id;

    msg_len = decode_cli_msg(f, msg);
    if (msg_len <= 0) {
        err(CLIENT, "decode message error");
        r = TRME_DECODE_ERR;
    } else {
        if (send(sock, msg, msg_len, 0) < 0) {
            err_n(CLIENT, "send failure");
            r = TRME_SOCKET_FAILURE;
        } else {
            q = m_alloc_s(struct active_query *);
            q->srv_sock = sock;
            q->timeout = FILE_TIMEOUT;
            q->transmission_id = transmission_id;
            q->status = SRV_BUSY;
            q->next = NULL;
            if (!queue->q_head)
                queue->q_head = q;
            if (queue->q_tail)
                queue->q_tail->next = q;
            queue->q_tail = q;
        }
    }
    return r;
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
    struct transmission *t = t_descr.trm + transmission_id;
    struct cli_fields f = { .error = 0 };

    strncpy(f.file_name, t->filename, FILE_NAME_MAX_LEN);
    memcpy(f.hsumm, t->filesum, MD5_DIGEST_LENGTH);

    for (i = 0, req_count = 0; i < q->count; i++) {
        if (require_piece(&f, transmission_id,  i, q->sockets[i]) == 0)
            req_count++;
    }
    if (!req_count) {
        r = TRME_NO_ACTIVE_SRVS;
        err(CLIENT, "no active servers found for start transmission");
    }
    return r;
}

/**
 * Запускает для передачи файл с определенной хэш-суммой.
 * Возвращает номер открытой передачи.
 * В случае ошибки возвращает отрицательное число, которое можно
 * интерпретировать с помощью TRME_* флагов
 */
int recieve_file(const char *filename, const unsigned char *hsum,
        const struct sockets_queue *q) {
    int tr_no, r;
    tr_no = add_transmission(filename, hsum);
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
void main_dispatcher(const struct sockets_queue *q) {
    if (q_descr.q_head) {
        process_timeouts();
        process_distrib();
    }
    response_servers(q);
}

/**
 * Callback-функция которая вызывается при получении сообщения клиентом.
 * Обрабатывает сообщение полученное от сервера.
 */
int process_srv_message(int sock, const char *msg, ssize_t len) {
    /* TODO: make server messages processing */
    log(CLIENT, "Recieved from server %d: %s", sock, msg);
    return 0;
}
