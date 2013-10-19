#ifndef PROTO_H
#define PROTO_H

#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include "macro.h"

typedef unsigned int pack_id_t;
#define PACK_ID_LENGTH sizeof(pack_id_t)
typedef unsigned int piece_num_t;
#define PIECE_NUM_LENGTH sizeof(piece_num_t)
typedef unsigned int file_num_t;
#define FILE_NUM_LENGTH sizeof(file_num_t)
typedef unsigned int piece_len_t;
#define PIECE_LEN_TSIZE sizeof(piece_len_t)


struct cli_fields {
    pack_id_t pack_id;
    piece_num_t piece_num;
    int piece_len;
    unsigned short error;
    char file_name[FILE_NAME_MAX_LEN];
    unsigned char hsumm[MD5_DIGEST_LENGTH];
};

struct srv_fields {
    struct cli_fields;
    unsigned char piece[BUF_MAX_LEN - sizeof(struct cli_fields)];
};

int encode_cli_msg(struct cli_fields *fields, const char *msg);
size_t decode_cli_msg(const struct cli_fields *fields, char *msg);
int encode_srv_msg(struct srv_fields *fields, const char *msg);
size_t decode_srv_msg(const struct srv_fields *fields, char *msg);


#endif
