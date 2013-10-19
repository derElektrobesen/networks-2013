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

#define ACT_DOWNLOAD_MSG        1
#define ACT_DOWNLOAD_MSG_ANSW   2
#define ACT_SEARCH_FILE         3
#define ACT_SEARCH_FILE_ANSW    4

#ifdef PROTO_DIVIDER
#   define DIVIDER_LENGTH 1
#else 
#   define DIVIDER_LENGTH 0
#endif


typedef unsigned int pack_id_t;
#define PACK_ID_LENGTH sizeof(pack_id_t)
typedef unsigned int piece_num_t;
#define PIECE_NUM_LENGTH sizeof(piece_num_t)
typedef unsigned int file_num_t;
#define FILE_NUM_LENGTH sizeof(file_num_t)

#define PROTO_ACTION_TSIZE 4

struct proto_fields {
    pack_id_t pack_id;
    unsigned int action_type;
    unsigned int msg_len;
    union {
        /* Запрос клиента на скачивание файла */
        struct {
            piece_num_t piece_num;
            file_num_t file_num;
            unsigned char sum[MD5_DIGEST_LENGTH];
        } act_download_piece;
        /* Ответ сервера на запрос на скачивание файла */
        struct {
            unsigned char sum[MD5_DIGEST_LENGTH];
            unsigned char data[BUF_MAX_LEN - 
                               CONTROL_INFO_LEN - 
                               MD5_DIGEST_LENGTH];
        } act_download_piece_answ;
        /* Запрос клиента на поиск файла */
        struct {
            char file_path[FILE_NAME_MAX_LEN];
        } act_search_file;
        /* Ответ сервера на запрос поиска файла */
        struct {
            unsigned char sum[MD5_DIGEST_LENGTH];
            unsigned int file_num;
            unsigned long file_size;
        } act_search_file_answ;
    };
};

int encode_msg(struct proto_fields *fields, const char *msg);
size_t decode_msg(const struct proto_fields *fields, char *msg);

#endif
