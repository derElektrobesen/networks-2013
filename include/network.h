#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <net/if.h>

#include "macro.h"
#include "types.h"

/* Constants */
#define SELECT_QUEUE_LEN        5
#define MAX_INTERFACES_COUNT    16
#define IDENT_MSG               "Dzhumagulov_Berezhnoy_IU_7_2013"

/* Types */
typedef int (*socket_callback)(int sender_sock,
        const char *recieved_data, size_t data_len);
typedef int (*server_response_callback)(struct sockets_queue *queue);

/* Prototypes */
int start_server(socket_callback process_cli_msg_callback);
int start_client(socket_callback process_srv_msg_callback,
        queue_dispatcher dispatcher, struct sockets_queue *q);
ssize_t send_data(int sock, char *buf, size_t len, int flags);

#endif
