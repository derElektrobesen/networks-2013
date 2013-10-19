#ifndef TYPES_H
#define TYPES_H

#include <arpa/inet.h>

/* Структура хранит массив активных сокетов */
struct sockets_queue {
    int sockets[MAX_CONNECTIONS];       /*< Массив сокетов              */
    in_addr_t addrs[MAX_CONNECTIONS];   /*< Массив активных подключений */
    int count;                          /*< Число акивных подключений   */
};

typedef void (*queue_dispatcher)(const struct sockets_queue *q);

#endif
