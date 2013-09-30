#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

#include "config.h"
#include "macro.h"

#define MAX_CONN 128

static const size_t BUF_MAX_LEN = 1024;
static const int MAX_CONNECTIONS = MAX_CONN;
static const int RETRY_TIMEOUT = 5;
static const char *IDENT_MSG = "Dzhumagulov_Berezhnoy_IU_7_2013";
static const char *message = "Bakit zadrot!";

struct sockets_queue {
    int sockets[MAX_CONN];
    in_addr_t addrs[MAX_CONN];
    int count;
    pthread_rwlock_t rwlock;
};

typedef int (*socket_callback)(int sender_sock, 
        const char *recieved_data, ssize_t data_len); 
typedef int (*server_answ_callback)(struct sockets_queue *queue);

#endif
