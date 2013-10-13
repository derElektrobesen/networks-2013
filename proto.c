#include "proto.h"

#define set_bit(val, pos)   ((unsigned int)((1 << (pos)) | (val)))
#define reset_bit(val, pos) ((unsigned int)((~(1 << (pos))) & (val)))
#define get_bit(val, pos)   ((unsigned int)((1 << (pos)) & (val)))

/**
 * Функция создает хеш.
 * sizeof(@md5digest) == MD5_DEGEST_LENGTH
 */
int get_hash(struct proto_fields *fields, unsigned char *md5digest) {
    unsigned char hash_key[2 * MAX_PACK_NUM_LEN + 1];
    MD5_CTX md5handler;

    if (fields == NULL) {
        err_n(SERVER, "get_hash failure");
        return 1;
    }
    /* TODO: Remove act_send_msg field from next line */
    //snprintf((char *)hash_key, sizeof(hash_key), "%d%d", fields->pack_num, fields->act_send_msg.piece_num);

    MD5_Init(&md5handler);
    MD5_Update(&md5handler, hash_key, sizeof(hash_key));
    MD5_Final(md5digest, &md5handler);

    return 0;
}

/**
 *
 */
int encode_msg(struct proto_fields *fields, char *msg) {
    unsigned char hash_msg[MD5_DIGEST_LENGTH];

    /* Fields filling */
    /*
    fields->pack_num = 0;
    fields->action_type = 0;
    fields->act_send_msg.piece_num = 0;
    fields->msg_len = strlen(msg);
    */

    get_hash(fields, hash_msg);
    if (hash_msg == NULL) {
        err_n(SERVER, "get hash failure");
        return -1;
    }

    /* TODO: Add serialization */
    return 0;
}

size_t decode_msg(const struct proto_fields *fields, const char *msg) {
    /* 1) Read fields
     * 2) Decoding message
     * 3) return msg_len
     */
    return 0;
}
