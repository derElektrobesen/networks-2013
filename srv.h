#ifndef SRV_H
#define SRV_H

#include <sys/socket.h>

#include "proto.h"

struct files_queue {
    char names[FILE_NAME_MAX_LEN][MAX_TRANSMISSIONS];
    int cur;
};

/* Обрабатывает сообщения, поступающие от клиентов */
int process_client_message(int sender_sock, const char *msg, size_t count);

#endif
