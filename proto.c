#include "proto.h"

#define set_bit(val, pos)   ((unsigned int)((1 << (pos)) | (val)))
#define reset_bit(val, pos) ((unsigned int)((~(1 << (pos))) & (val)))
#define get_bit(val, pos)   ((unsigned int)((1 << (pos)) & (val)))

/**
 * tункция создает хеш.
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

    /*MD5_Init(&md5handler);
    MD5_Update(&md5handler, hash_key, sizeof(hash_key));
    MD5_Final(md5digest, &md5handler);
*/
    return 0;
}

unsigned int get_hex(const char *msg, int n) {
    unsigned int result = 0;
    int i = 0;
    for (i = 0; i < n; i++) {
        result <<= 8;
        result |= (unsigned char)msg[i];
    }
    return result;
}

/**
 * Заполнение структуры proto_fields
 */
int encode_msg(struct proto_fields *fields, const char *msg) {
    unsigned char hash_msg[MD5_DIGEST_LENGTH];
    int i;
    unsigned int buf;
    const char *p;

    if (fields == NULL) {
        err(OTHER, "incorrect protocol");
        return -1;
    }
    unsigned int action = get_hex(msg, PROTO_ACTION_TSIZE);
    if (action <= 0) {
        err(OTHER, "Incorrect action type");
        return -1;
    }

    if (action == ACT_DOWNLOAD_MSG) {
        /* Action type */
        fields->action_type = action;
        p = msg + PROTO_ACTION_TSIZE + DIVIDER_LENGTH;
        /* Packet id */
        fields->pack_id = get_hex(p, PACK_ID_LENGTH);
        /* Message length */
        p += PACK_ID_LENGTH + DIVIDER_LENGTH;
        fields->msg_len = get_hex(p, BUF_MAX_LEN_TSIZE);
        /* Data */
        p += BUF_MAX_LEN_TSIZE + DIVIDER_LENGTH;
        fields->act_download_piece.piece_num = get_hex(p, 
                                            PIECE_NUM_LENGTH);
        p += PIECE_NUM_LENGTH + DIVIDER_LENGTH;
        fields->act_download_piece.file_num = get_hex(p, 
                                            FILE_NUM_LENGTH);
        p += FILE_NUM_LENGTH + DIVIDER_LENGTH;
        for (i = 0, p++; i < MD5_DIGEST_LENGTH; i++, p++)
            fields->act_download_piece.sum[i] = *p;
    }
    if (action == ACT_DOWNLOAD_MSG_ANSW) {
    }
     
    
    
    get_hash(fields, hash_msg);
    if (hash_msg == NULL) {
        err_n(SERVER, "get hash failure");
        return -1;
    }

    return 0;
}

size_t decode_msg(const struct proto_fields *fields,  char *msg) {
    /* 1) Read fields
     * 2) Decoding message
     * 3) return msg_len
     */
    return 0;
}
