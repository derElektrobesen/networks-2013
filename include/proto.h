#ifndef PROTO_H
#define PROTO_H

#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "macro.h"

typedef unsigned int pack_id_t;
typedef unsigned int piece_id_t;
typedef signed int file_id_t;
typedef unsigned short perror_t;

#define PACK_ID_TSIZE sizeof(pack_id_t)
#define PIECE_NUM_TSIZE sizeof(piece_id_t)
#define FILE_NUM_TSIZE sizeof(file_id_t)
#define PIECE_LEN_TSIZE sizeof(size_t)
#define PROTOCOL_ERROR_TSIZE sizeof(perror_t)

/**
 * Номера битов, которые отвечают за конкретные ошибки
 */
#define PE_FOPEN_FAILURE        0
#define PE_FILE_NOT_EXISTS      1
#define PE_HASH_CMP_FAILURE     2
#define PE_TRNMS_CMPL           3       /** Передача завершена */
#define PE_READ_ACCESS_DENIED   4

struct cli_fields {
    pack_id_t pack_id;
    piece_id_t piece_id;
    perror_t error;
    file_id_t file_id;
    unsigned char hsumm[MD5_DIGEST_LENGTH];
    char file_name[FILE_NAME_MAX_LEN];
};

#define DATA_BLOCK_LEN (BUF_MAX_LEN - sizeof(struct cli_fields) - PIECE_LEN_TSIZE)

struct srv_fields {
    struct cli_fields cli_field;
    size_t piece_len;
    unsigned char piece[DATA_BLOCK_LEN];
};

int encode_cli_msg(struct cli_fields *fields, const char *msg, size_t msg_len);
size_t decode_cli_msg(const struct cli_fields *fields, char *msg);
int encode_srv_msg(struct srv_fields *fields, const char *msg, size_t msg_len);
size_t decode_srv_msg(const struct srv_fields *fields, char *msg);

void decode_proto_error(perror_t e, char *s, int max_len);

#endif
