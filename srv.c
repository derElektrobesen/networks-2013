#include "srv.h"

static struct files_queue f_descr = {
    .cur = 0,
};

static void process_cli_msg(struct srv_fields *f) {

}

static void send_answer(struct srv_fields *f, int sock) {

}

int process_client_message(int sender_sock, const char *msg, size_t count) {
    struct srv_fields f;
    int r;

    r = encode_cli_msg(&(f.cli_field), msg, count);
    if (r)
        err(SERVER, "Encoding cli message failure");
    else {
        process_cli_msg(&f);
        send_answer(&f, sender_sock);
    }
    return r;
}
