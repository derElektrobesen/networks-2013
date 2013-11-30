#ifndef TYPES_H
#define TYPES_H

#include <arpa/inet.h>

/**
 * Структура хранит массив активных сокетов
 */
struct sockets_queue {
    int sockets[MAX_CONNECTIONS];       /*< Массив сокетов              */
    in_addr_t addrs[MAX_CONNECTIONS];   /*< Массив активных подключений */
    int count;                          /*< Число акивных подключений   */
};

typedef void (*queue_dispatcher)();
typedef void (*gui_recv_callback)(char *const *, char *const *, unsigned int);
typedef void (*gui_send_callback)(const char *);

struct gui_actions {
    gui_recv_callback start_trm;
    gui_recv_callback stop_trm;
    gui_recv_callback terminate;
    gui_send_callback package_sent;
    gui_send_callback package_recieved;
    gui_send_callback server_added;
    gui_send_callback client_added;
    gui_send_callback server_removed;
    gui_send_callback client_removed;
    int sock;
};

#endif
