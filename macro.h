#ifndef MACRO_H
#define MACRO_H

#include "config.h"

/* For logging */
#define SERVER 0
#define CLIENT 1

#ifdef DEBUG
#   define print_cli(cli) \
    (cli == SERVER ? "Server" : (cli == CLIENT ? "Client" : "Other"))

#   define log(cli, f_str, args...) \
        fprintf(stdout, "[ %s ] " f_str "\n", print_cli(cli), ##args)
#   define err(cli, f_str, args...) \
        fprintf(stderr, "[ %s ] " f_str "\n", print_cli(cli), ##args)
#   define err_n(cli, f_str, args...) \
        err(cli, f_str ": %s", strerror(errno), ##args) /* \n don't needed */

#else /* DEBUG is undefined */
#   define log(...)
#   define err(...)
#   define err_n(...)
#endif /* DEBUG */

#endif
