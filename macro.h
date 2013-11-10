#ifndef MACRO_H
#define MACRO_H

/* For logging */
#define OTHER      -1
#define SERVER      0
#define CLIENT      1
#define BROADCAST   2

#ifdef DEBUG
#   define print_cli(cli) \
    (cli == SERVER ? "Server" : \
    (cli == CLIENT ? "Client" : \
    (cli == BROADCAST ? "Broadcast" : "Other")))

#   ifdef PRINT_LINES
#       define log(cli, f_str, args...) \
            fprintf(stdout, "[ %9s ] " f_str " at line %d\n", print_cli(cli), ##args, __LINE__)
#       define err(cli, f_str, args...) \
            fprintf(stderr, "[ %9s ] " f_str " at line %d\n", print_cli(cli), ##args, __LINE__)
#   else
#       define log(cli, f_str, args...) \
            fprintf(stdout, "[ %9s ] " f_str "\n", print_cli(cli), ##args)
#       define err(cli, f_str, args...) \
            fprintf(stderr, "[ %9s ] " f_str "\n", print_cli(cli), ##args)
#   endif

#   define err_n(cli, f_str, args...) \
        err(cli, f_str " : %s", ##args, strerror(errno)) /* \n don't needed */
#   define locate log(OTHER, "%s, %d", __FUNCTION__, __LINE__)
#   define convert_hex_str(buf, buflen, str, len) {             \
        int i; char *ptr = (buf);                               \
        for (i = 0; i < (len); i++) {                           \
            snprintf(ptr, (buflen) - i * 3, "%02x", (str)[i]);  \
            ptr += 2;                                           \
        }                                                       \
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
                   "error: %d, hsumm:%s, fname: %s",           \
                   (f_ptr)->pack_id, (f_ptr)->piece_id,         \
                   (f_ptr)->file_id, (f_ptr)->error,            \
                   r, (f_ptr)->file_name);                      \
    }
#   define log_srv_fields(f_ptr) {                              \
        char r[255];                                            \
        convert_hex_str(r, sizeof(r), (f_ptr)->cli_field.hsumm, MD5_DIGEST_LENGTH); \
        log(OTHER, "pack_id: %d, piece_id: %d, file_id: %d, "   \
                   "error: %d, piece_len: %lu, hsumm:%s, fname: %s", \
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

#endif
