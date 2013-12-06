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
#include <sys/un.h>

#include "macro.h"
#include "types.h"
#include "json.h"

/* Constants */
#define SELECT_QUEUE_LEN        5
#define MAX_INTERFACES_COUNT    16
#define IDENT_MSG               "Dzhumagulov_Berezhnoy_IU_7_2013"

struct message {
    char message[BUF_MAX_LEN];
    int bytes_count;
    int bytes_read;
};

/* Types */
typedef int (*socket_callback)(int sender_sock,
        const char *received_data, size_t data_len);
typedef int (*server_response_callback)(struct sockets_queue *queue);

/* Prototypes */
int start_server(socket_callback process_cli_msg_callback);
int start_client(socket_callback process_srv_msg_callback, queue_dispatcher dispatcher, struct sockets_queue *q);
int send_data(int sock, char *buf, int len, int flags);
void setup_gui_msgs(struct gui_actions *acts);

#endif
