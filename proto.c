#include "proto.h"

#define ITOC_MAX 12

/* Функция преобразует целое положительное 
 * число в строковое представление.
 */
char *itoc(int n) {
    if (n < 0)
        return NULL;
    char *result = (char *)malloc(ITOC_MAX);
    int i = 0;
    do {
        *(result + i) = n % 10 + '0';
    } while((n/=10) > 0);
    *(result + i) = 0;
    return result;
    /*without reverse*/
}

/* Функция создает ключ для хеширования.
 */
unsigned char *get_hash_key(struct proto_fields *fields) {
    if (fields == NULL) {
        err_n(SERVER, "incorrect protocol");
        return NULL;
    }
    unsigned char *key1 = (unsigned char *)itoc(fields->pack_num);
    unsigned char *key2 = 
        (unsigned char *)itoc(fields->act_send_msg.piece_num);    
    unsigned char *hash_key = malloc(sizeof(key1) + sizeof(key2) + 1);
    hash_key = memcpy(hash_key, key1, sizeof(key1));
    hash_key = memcpy(hash_key, key2, sizeof(key2));
    free(key1);
    free(key2);
    return hash_key;
}

/* Функция создает хеш.
 */
unsigned char *get_hash(struct proto_fields *fields) {

    if (fields == NULL) {
        err_n(SERVER, "get_hash failure");
        return NULL;
    }
    unsigned char *hash_key = get_hash_key(fields);
    unsigned char *md5digest = malloc(MD5_DIGEST_LENGTH);
    MD5_CTX md5handler;
    MD5_Init(&md5handler);
    MD5_Update(&md5handler, hash_key, sizeof(hash_key));
    MD5_Final(md5digest, &md5handler);
    
    free(hash_key);
    return md5digest;
}
/* Установка бита
 */
unsigned int set_bit(unsigned int val, int pos) {
    if (val < 0 || pos < 0) {
        err_n(SERVER, "incorrect bit position or value mean");
        return -1;
    }
    return (unsigned int)((1 << pos) | val);
}
/* Сброс бита 
 */
unsigned int reset_bit(unsigned int val, int pos) {
    if (val < 0 || pos < 0) {
        err_n(SERVER, "incorrect bit postition or value mean");
        return -1;
    }   
    return (unsigned int)((~(1 << pos)) & val);
}

/*
 */
int encode_msg(struct proto_fields *fields, char *msg) {
    /* Fields filling */

    fields->pack_num = 0;
    fields->action_type = 0;
    fields->act_send_msg.piece_num = 0;
    fields->msg_len = strlen(msg); 

    unsigned char *hash_msg = get_hash(fields);
    if (hash_msg == NULL) {
        err_n(SERVER, "get hash failure");
        return -1;
    }

    /* Add serialization */



    free(hash_msg);
    return 0;
}

size_t decode_msg(const struct proto_fields *fields, const char *msg) {
    /* 1) Read fields
     * 2) Decoding message
     * 3) return msg_len
     */
    return 0;
}
