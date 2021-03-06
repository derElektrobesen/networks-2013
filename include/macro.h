#ifndef MACRO_H
#define MACRO_H

#ifdef DAEMONIZE
#   include <syslog.h>
#   define LOG_FUNC syslog
#   define LOG_CMD  LOG_INFO
#   define ERR_CMD  LOG_ERR
#else
#   define OPEN_LOG
#   define LOG_FUNC fprintf
#   define LOG_CMD  stdout
#   define ERR_CMD  stderr
#endif

/* For logging */
#define OTHER      -1
#define SERVER      0
#define CLIENT      1
#define BROADCAST   2

#define print_cli(cli) \
    (cli == SERVER ? "Server" : \
    (cli == CLIENT ? "Client" : \
    (cli == BROADCAST ? "Broadcast" : "Other")))

#ifdef PRINT_LINES
#    define log(cli, f_str, args...) { \
        LOG_FUNC(LOG_CMD, "[ %9s ] " f_str " at line %d\n", print_cli(cli), ##args, __LINE__); }
#    define err(cli, f_str, args...) { \
        LOG_FUNC(ERR_CMD, "[ %9s ] " f_str " at line %d\n", print_cli(cli), ##args, __LINE__); }
#else
#   define log(cli, f_str, args...) { \
        LOG_FUNC(LOG_CMD, "[ %9s ] " f_str "\n", print_cli(cli), ##args); }
#   define err(cli, f_str, args...) { \
        LOG_FUNC(ERR_CMD, "[ %9s ] ERROR ------------------------> " f_str "\n", print_cli(cli), ##args); }
#endif

#ifdef DEBUG
#   define err_n(cli, f_str, args...) \
        err(cli, f_str " : %s", ##args, strerror(errno)) /* \n don't needed */
#   define locate log(OTHER, "%s, %d", __FUNCTION__, __LINE__)
#   define convert_hex_str(buf, buflen, str, len) {             \
        int i;                                                  \
        for (i = 0; i < (len); i++)                             \
            snprintf((buf) + i * 2, (buflen) - i * 2, "%02x", (int)*((str) + i)); \
        *((buf) + 2 * (len) + 1) = 0;                           \
    }
#   define print_hex_str(msg, str, len) {                       \
        char r[255];                                            \
        convert_hex_str(r, sizeof(r), str, len);                \
        log(OTHER, "%s: %s", msg, r);                           \
    }
#   define log_cli_fields(f_ptr) {                              \
        char r[255];                                            \
        convert_hex_str(r, sizeof(r), (f_ptr)->hsumm, MD5_DIGEST_LENGTH); \
        log(OTHER, "pack_id: %d, piece_id: %d, file_id: %d, "   \
                   "error: %d, hsumm: %s, fname: %s",           \
                   (f_ptr)->pack_id, (f_ptr)->piece_id,         \
                   (f_ptr)->file_id, (f_ptr)->error,            \
                   r, (f_ptr)->file_name);                      \
    }
#   define log_srv_fields(f_ptr) {                              \
        char r[255];                                            \
        char data[DATA_BLOCK_LEN + 1];                          \
        memcpy(data, (f_ptr)->piece, DATA_BLOCK_LEN);           \
        data[DATA_BLOCK_LEN] = 0;                               \
        convert_hex_str(r, sizeof(r), (f_ptr)->cli_field.hsumm, MD5_DIGEST_LENGTH); \
        log(OTHER, "pack_id: %d, piece_id: %d, file_id: %d, "   \
                   "error: %d, piece_len: %llu, hsumm:%s, fname: %s", \
                   (f_ptr)->cli_field.pack_id,                  \
                   (f_ptr)->cli_field.piece_id,                 \
                   (f_ptr)->cli_field.file_id,                  \
                   (f_ptr)->cli_field.error,                    \
                   (f_ptr)->piece_len,                          \
                   r,                                           \
                   (f_ptr)->cli_field.file_name);               \
    }

#else  /* DEBUG is undefined */
#   define log(...)
#   define err(...)
#   define err_n(...)
#   define locate
#   define log_cli_fields(...)
#   define log_srv_fields(...)
#   define convert_hex_str(...)
#   define print_hex_str(...)
#endif /* DEBUG */

#define m_alloc(type, count) ((type *)malloc(sizeof(type) * (count)))
#define m_alloc_s(type) (m_alloc(type, 1)) /* Allocate single obj */

#define set_bit(val, pos)   ((unsigned int)((1 << (pos)) | (val)))
#define reset_bit(val, pos) ((unsigned int)((~(1 << (pos))) & (val)))
#define get_bit(val, pos)   ((unsigned int)((1 << (pos)) & (val)))

#define st_arr_len(arr_ptr) (sizeof(arr_ptr) / sizeof(*(arr_ptr)))

#define TO_STR(name)        #name               /* macro name to str */

#endif
