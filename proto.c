#include "proto.h"

/**
 * tункция создает хеш.
 * sizeof(@md5digest) == MD5_DEGEST_LENGTH
 */
int get_hash(struct cli_fields *fields, unsigned char *md5digest) {

    /* TODO: Remove act_send_msg field from next line */
    //snprintf((char *)hash_key, sizeof(hash_key), "%d%d", fields->pack_num, fields->act_send_msg.piece_id);

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
 * Заполнение структуры cli_fields клиентом
 * Протокол получен от сервера
 */
int encode_cli_msg(struct cli_fields *fields, const char *msg, ssize_t msg_len) {
    int r = 0;
    unsigned int e = 0;
    const char *p = msg;
    memcpy(&e, p, PROTOCOL_ERROR_TSIZE);
    if (e != 0){
        err(CLIENT, "bad package");
        r = -1;
    } else {
        memcpy(&(fields->pack_id), p+=PROTOCOL_ERROR_TSIZE, PACK_ID_TSIZE); 
        memcpy(&(fields->piece_id), p+=PACK_ID_TSIZE, PIECE_NUM_TSIZE);
        fields->error = e;
        memcpy(&(fields->file_id), p+=PIECE_NUM_TSIZE, FILE_NUM_TSIZE);
        memcpy(&(fields->hsumm), p+= FILE_NUM_TSIZE, MD5_DIGEST_LENGTH);
        memcpy(&(fields->file_name), p+=MD5_DIGEST_LENGTH, FILE_NAME_MAX_LEN);
    }
    return r;
}


/**
 * Заполнение структуры srv_fields сервером
 * Протокол получен от клиента
 */
int encode_srv_msg(struct srv_fields *fields, const char *msg, ssize_t msg_len) {
    int r = 0;
    unsigned int e = 0;
    const char *p = msg;
    memcpy(&e, p, PROTOCOL_ERROR_TSIZE);
    if (e != 0) {
        err(CLIENT, "bad protocol");
        r = -1;
    } else {
        
        memcpy(&(fields->cli_field.pack_id), p+=PROTOCOL_ERROR_TSIZE,
                PACK_ID_TSIZE); 
        memcpy(&(fields->cli_field.piece_id), p+=PACK_ID_TSIZE, PIECE_NUM_TSIZE);
        fields->cli_field.error = e;
        memcpy(&(fields->piece_len), p+=PIECE_NUM_TSIZE, PIECE_LEN_TSIZE);
        memcpy(&(fields->cli_field.file_id), p+=PIECE_LEN_TSIZE, FILE_NUM_TSIZE);
        memcpy(&(fields->cli_field.hsumm), p+=FILE_NUM_TSIZE, MD5_DIGEST_LENGTH);
        memcpy(&(fields->cli_field.file_name), p+= MD5_DIGEST_LENGTH, 
                FILE_NAME_MAX_LEN);
        memcpy(&(fields->piece), p+=FILE_NAME_MAX_LEN, 
                (BUF_MAX_LEN - sizeof(struct cli_fields)));
    }
    return r;
}

/**
 * Формирование сообщения клиентом
 * Передается серверу
 */
size_t decode_cli_msg(const struct cli_fields *fields,  char *msg) {
    size_t msg_length;
    char *p_start, *p_end;

    if (fields == NULL) {
        err(CLIENT, "incorrect protocol");
        return -1;
    }
    msg_length = 0;
    p_start = msg;
    memcpy(msg, &(fields->error), PROTOCOL_ERROR_TSIZE);
    memcpy(msg+=PROTOCOL_ERROR_TSIZE, &(fields->pack_id), PACK_ID_TSIZE);
    memcpy(msg+=PACK_ID_TSIZE, &(fields->piece_id), PIECE_NUM_TSIZE);
    memcpy(msg+=PIECE_NUM_TSIZE, &(fields->file_id), FILE_NUM_TSIZE);
    memcpy(msg+=FILE_NUM_TSIZE, &(fields->hsumm), MD5_DIGEST_LENGTH);
    msg = strncpy(msg+=MD5_DIGEST_LENGTH,
            fields->file_name,FILE_NAME_MAX_LEN);
    p_end = msg + FILE_NAME_MAX_LEN;
    msg_length = (p_end - p_start) / sizeof(char);
    return msg_length;
}

/**
 * Формирование сообщения (протокола) сервером
 */
size_t decode_srv_msg(const struct srv_fields *fields, char *msg) {
    size_t msg_length;
    char *p_start, *p_end;

    if (fields == NULL) {
        err(SERVER, "incorrect protocol");
        return -1;
    }
    msg_length = 0;
    p_start = msg;
    memcpy(msg, &(fields->cli_field.error), PROTOCOL_ERROR_TSIZE);
    memcpy(msg+=PROTOCOL_ERROR_TSIZE,
            &(fields->cli_field.pack_id), PACK_ID_TSIZE);
    memcpy(msg+=PACK_ID_TSIZE,
            &(fields->cli_field.piece_id), PIECE_NUM_TSIZE);
    memcpy(msg+=PIECE_NUM_TSIZE, &(fields->piece_len), PIECE_LEN_TSIZE);
    memcpy(msg+=PIECE_LEN_TSIZE, &(fields->cli_field.file_id),
            FILE_NUM_TSIZE);
    memcpy(msg+=FILE_NUM_TSIZE,
            &(fields->cli_field.hsumm), MD5_DIGEST_LENGTH);
    msg = strncpy(msg+=MD5_DIGEST_LENGTH,
            fields->cli_field.file_name, FILE_NAME_MAX_LEN);
    memcpy(msg+=FILE_NAME_MAX_LEN, fields->piece,
            BUF_MAX_LEN - sizeof(struct cli_fields));
    p_end = msg + BUF_MAX_LEN - sizeof(struct cli_fields);
    msg_length = (p_end - p_start) / sizeof(char);
    return msg_length;
}

