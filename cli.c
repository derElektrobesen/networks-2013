#include "cli.h"

#define m_alloc(type, count) ((type)malloc(sizeof(type) * (count)))
#define m_alloc_s(type) (m_alloc(type, 1)) /* Allocate single obj */

static struct active_queries_descr q_descr;
static struct sockets_queue *socks;
static const char *message = "Bakit zadrot!";

static int process_timeouts() {
    struct active_queries *q = &(q_descr.q_head);
    /* int i; */
    do {
        if (q->status == SRV_BUSY && --(q->timeout) == 0) {
            /* Timeout came */
            q->status = SRV_READY;
            /*
            i = --(q->pieces->cur_elem);
            q->pieces->pieces[i] = q->fields.act_send_msg.piece_num;
            */
        }
        q = q->next;
    } while (q);
    return 0;
}

static int process_distrib() {
    struct active_queries *q = &(q_descr.q_head);
    char msg[BUF_MAX_LEN];
    size_t msg_len;
    /* int i; */
    do {
        /* Send new requests to free active_queries */
        if (q->status == SRV_READY) {
            q->timeout = FILE_TIMEOUT;
            /*
            q->fields.pack_num = (int)random();
            i = q->pieces->cur_elem;
            q->fields.act_send_msg.piece_num = q->pieces->pieces[i];
            */
            msg_len = decode_msg(&(q->fields), msg);
            if (msg_len <= 0) {
                err(CLIENT, "Message decoding failure");
            } else {
                (q->pieces->cur_elem)++;
                q->status = SRV_BUSY;
                send(q->srv_sock, msg, msg_len, 0);
            }
        }
    } while (q);
    return 0;
}

/* Функция рассылающая сообщения серверам в локальной сети */
static int response_servers() {
    int i;

    /* TODO: Remove dummy actions */
    for (i = 0; i < socks->count; i++)
        send(socks->sockets[i], message, strlen(message), 0);

    return 0;
}

/*
 * Обрабатывает список активных передач.
 */
static void main_handler(int sig) {
    if (q_descr.q_head.status != SRV_UNKN) {
        process_timeouts();
        process_distrib();
    }
    response_servers();
    alarm(ALARM_DELAY);
}

int set_client_alarm(struct sockets_queue *q) {
#ifndef DONT_DO_SRAND
    srandom(time(NULL));
#else
    srandom(1);
#endif
    signal(SIGALRM, &main_handler);

    q_descr.q_head.next = NULL;
    q_descr.q_head.status = SRV_UNKN;
    q_descr.q_tail = NULL;

    socks = q;

    alarm(ALARM_DELAY);

    return 0;
}

/*
int recieve_file(const char *filename, int sock,
                 struct pieces_queue *pieces) {
    struct active_queries *q = m_alloc_s(struct active_queries *);
    q->srv_sock = sock;
    q->timeout = FILE_TIMEOUT / ALARM_DELAY;
    q->status = SRV_READY;
    q->pieces = pieces;
    if (q_descr.qtail)
        q_descr.qtail->next = q;
    else {
        q_descr.q_tail = q;
        q_descr.q_head.next = q;
    }
    return 0;
}
*/

/*
 * Callback-функция которая вызывается при получении сообщения клиентом.
 * Обрабатывает сообщение полученное от сервера.
 */
int process_srv_message(int sock, const char *msg, ssize_t len) {
    /* TODO: make server messages processing */
    log(CLIENT, "Recieved from server %d: %s", sock, msg);
    return 0;
}
