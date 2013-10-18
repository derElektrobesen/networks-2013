#ifndef CLI_H
#define CLI_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef DONT_DO_SRAND
#   include <time.h>
#endif

#include "proto.h"
#include "macro.h"
#include "types.h"

/* Defines */
#define SRV_UNKN  0
#define SRV_READY 1
#define SRV_BUSY  2

/* Typedefs */
struct active_queries {
    int srv_sock;
    int timeout;
    unsigned short status;
    struct proto_fields fields;
    struct pieces_queue *pieces;
    struct active_queries *next;
};

struct active_queries_descr {
    struct active_queries q_head;
    struct active_queries *q_tail;
};

/* Часть файла которую необходимо запросить */
struct pieces_queue {
    int max_piece_num;      /**< Максимальный номер куска */
    int cur_max_piece_num;  /**< Максимальный номер куска в pieces*/
    int cur_elem;           /**< Минимальный индекс в pieces*/
    int pieces[MAX_PIECES_COUNT];
};

/* Functions prototypes */
/* Обрабатывает сообщение полученное от сервера */
int process_srv_message(int sock, const char *msg, ssize_t len);
/* Основной диспетчер */
void main_dispatcher(struct sockets_queue *q);

/*
int recieve_file(const char *filename);
*/
#endif
