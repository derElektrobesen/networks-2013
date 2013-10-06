#include "cli.h"

static struct active_servers srv_head;

static int process_timeouts() {
    struct active_servers *srv = &srv_head;
    int i;
    do {
        if (srv->status == SRV_BUSY && --(srv->timeout) == 0) {
            /* Timeout came */
            srv->status = SRV_READY;
            i = --(srv->pieces->cur_elem);
            srv->pieces->pieces[i] = srv->fields.act_send_msg.piece_num;
        }
        srv = srv->next;
    } while (srv);
    return 0;
}

static int process_distrib() {
    struct active_servers *srv = &srv_head;
    char msg[BUF_MAX_LEN];
    size_t msg_len;
    int i;
    do {
        /* Send new requests to free active_servers */
        if (srv->status == SRV_READY) {
            srv->timeout = FILE_TIMEOUT;
            srv->fields.pack_num = (int)random();
            i = srv->pieces->cur_elem;
            srv->fields.act_send_msg.piece_num = srv->pieces->pieces[i];
            msg_len = decode_msg(&(srv->fields), msg);
            if (msg_len <= 0) {
                err(CLIENT, "Message decoding failure");
            } else {
                (srv->pieces->cur_elem)++;
                srv->status = SRV_BUSY;
                send(srv->srv_sock, msg, msg_len, 0);
            }
        }
    } while (srv);
    return 0;
}

/*
 * Обрабатывает список активных передач.
 */
static void main_handler(int sig) {
    if (srv_head.status != SRV_UNKN) {
        process_timeouts();
        process_distrib();
    }
}

int set_client_alarm() {
#ifndef DONT_DO_SRAND
    srandom(time(NULL));
#endif
    signal(SIGALRM, &main_handler);

    srv_head.next = NULL;
    srv_head.status = SRV_UNKN;

    alarm(ALARM_DELAY);

    return 0;
}

int recieve_file(const char *filename) {
    return 0;
}
