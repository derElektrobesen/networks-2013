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

#define ACT_SEND_MSG 1

struct proto_fields {
    int pack_num;
    int action_type;
    int msg_len;/* ? */
    union {
    struct {
            int piece_num;
            int file_num;
        } act_send_msg;
    };
};

int encode_msg(struct proto_fields *fields, char *msn);
size_t decode_msg(const struct proto_fields *fields, const char *msg);

#endif
