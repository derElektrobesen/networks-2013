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

static const size_t BUF_MAX_LEN = 1024;
static const int MAX_CONNECTIONS = 128;
static const int RETRY_TIMEOUT = 5;
static const char *IDENT_MSG = "Dzhumagulov_Berezhnoy_IU_7_2013";

typedef int (*socket_callback)(int sender_sock, 
        const char *recieved_data, ssize_t data_len); 

#endif
