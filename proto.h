#ifndef PROTO_H
#define PROTO_H

#include <stddef.h>

#define ACT_SEND_MSG 1

struct proto_fields {
    int pack_num;
    int action_type;
    union {
        struct {
            int piece_num;
            int file_num;
        } act_send_msg;
    };
};

int encode_msg(struct proto_fields *fields, const char *msg, size_t msg_len);
size_t decode_msg(const struct proto_fields *fields, char *msg);

#endif
