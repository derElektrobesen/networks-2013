#include "srv.h"

int process_client_message(int sender_sock, const char *msg, ssize_t count) {
    char *message = "Message recieved!\n";
    log(SERVER, "message recieved from socket %d: %s", sender_sock, msg);
    send(sender_sock, message, strlen(message), 0);
    return 0;
}
