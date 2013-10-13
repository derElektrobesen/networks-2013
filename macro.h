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
#   define check_rwlock(cli, rc, fname) \
        rc == 0 ? 0 : err_n(cli, fname " failure");

#else  /* DEBUG is undefined */
#   define log(...)
#   define err(...)
#   define err_n(...)
#   define check_rwlock(...)
#endif /* DEBUG */

#endif
