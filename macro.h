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
#   define log_cli_fields(f_ptr) \
        log(OTHER, "pack_id: %d, piece_id: %d, file_id: %d, "   \
                   "error: %d, hsumm: %.16s, fname: %s",        \
                   (f_ptr)->pack_id, (f_ptr)->piece_id,         \
                   (f_ptr)->file_id, (f_ptr)->error,            \
                   (f_ptr)->hsumm, (f_ptr)->file_name)

#   define log_srv_fields(f_ptr) \
        log(OTHER, "pack_id: %d, piece_id: %d, file_id: %d, "   \
                   "error: %d, piece_len: %lu, hsumm: %.16s, fname: %s", \
                   (f_ptr)->cli_field.pack_id,                  \
                   (f_ptr)->cli_field.piece_id,                 \
                   (f_ptr)->cli_field.file_id,                  \
                   (f_ptr)->cli_field.error,                    \
                   (f_ptr)->piece_len,                          \
                   (f_ptr)->cli_field.hsumm,                    \
                   (f_ptr)->cli_field.file_name)
#   define print_hex_str(msg, str, len) {                       \
        int i; char r[255]; char *ptr = r;                      \
        for (i = 0; i < len; i++) {                             \
            snprintf(ptr, 255 - i * 3, " %2x", str[i]);         \
            ptr += 3;                                           \
        }                                                       \
        log(OTHER, "%s: %s", msg, r);                           \
    }

#else  /* DEBUG is undefined */
#   define log(...)
#   define err(...)
#   define err_n(...)
#   define locate
#   define log_cli_fields(...)
#   define log_srv_fields(...)
#   define print_hext_str(...)
#endif /* DEBUG */

#define m_alloc(type, count) ((type *)malloc(sizeof(type) * (count)))
#define m_alloc_s(type) (m_alloc(type, 1)) /* Allocate single obj */

#define set_bit(val, pos)   ((unsigned int)((1 << (pos)) | (val)))
#define reset_bit(val, pos) ((unsigned int)((~(1 << (pos))) & (val)))
#define get_bit(val, pos)   ((unsigned int)((1 << (pos)) & (val)))

#endif
