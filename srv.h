#ifndef SRV_H
#define SRV_H

#include <sys/socket.h>

#include "proto.h"

/* Обрабатывает сообщения, поступающие от клиентов */
int process_client_message(int sender_sock, const char *msg, ssize_t count);

#endif
