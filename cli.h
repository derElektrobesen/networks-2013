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

/* Defines */
#define SRV_UNKN  0
#define SRV_READY 1
#define SRV_BUSY  2

/* Typedefs */
struct servers {
    int srv_sock;
    int timeout;
    unsigned short status;
    struct proto_fields fields;
    struct pieces_queue *pieces;
    struct servers *next;
};

struct pieces_queue {
    int max_piece_num;
    int cur_max_piece_num;
    int cur_elem;
    int pieces[MAX_PIECES_COUNT];
};

/* Functions prototypes */
int set_client_alarm();
int recieve_file(const char *filename);

#endif
