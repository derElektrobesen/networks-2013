#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"

#ifdef IP_V4        /* IP ver.4 */
#   define INET_ADDR_LEN    4
#elif defined IP_V6 /* IP ver.6 */
#   define INET_ADDR_LEN    16
#else
    perror("Proto is required\n");
#endif

typedef char[INET_ADDR_LEN] inet_addr;

#endif
