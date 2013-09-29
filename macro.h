#ifndef MACRO_H
#define MACRO_H

#include "config.h"

/* For logging */
#define SERVER      0
#define CLIENT      1
#define BROADCAST   2

#ifdef DEBUG
#   define print_cli(cli) \
    (cli == SERVER ? "Server" : \
    (cli == CLIENT ? "Client" : \
    (cli == BROADCAST ? "Broadcast" : "Other")))

#   define log(cli, f_str, args...) \
        fprintf(stdout, "[ %9s ] " f_str "\n", print_cli(cli), ##args)
#   define err(cli, f_str, args...) \
        fprintf(stderr, "[ %9s ] " f_str " at line %d\n", print_cli(cli), ##args, __LINE__)
#   define err_n(cli, f_str, args...) \
        err(cli, f_str " : %s", strerror(errno), ##args) /* \n don't needed */
#   define check_rwlock(cli, rc, fname) \
        rc == 0 ? err(cli, fname " failure") : 0;

#else /* DEBUG is undefined */
#   define log(...)
#   define err(...)
#   define err_n(...)
#endif /* DEBUG */

#endif
