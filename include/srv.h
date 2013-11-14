#ifndef SRV_H
#define SRV_H

#include <sys/socket.h>

#include "proto.h"

struct file_cache {
    unsigned char data[CACHED_PIECES_COUNT * DATA_BLOCK_LEN];
    piece_id_t start_piece;
    piece_id_t end_piece;
    char name[FILE_NAME_MAX_LEN];
    FILE *file;
};

struct files_queue {
    char positions[MAX_TRANSMISSIONS]; /* 0 (free) & 1 (reserved) */
    struct file_cache cache[MAX_TRANSMISSIONS];
    int count;
};

/* Обрабатывает сообщения, поступающие от клиентов */
int process_client_message(int sender_sock, const char *msg, size_t count);
ssize_t send_data(int sock, char *buf, size_t len, int flags);

#endif
