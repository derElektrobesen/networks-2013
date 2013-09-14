#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

#include "config.h"

#ifdef IP_V4        /* IP ver.4 */
    typedef uint32_t inet_ip_addr;
#elif defined IP_V6 /* IP ver.6 */
    typedef char inet_ip_addr[16];
#else
    perror("Proto is required\n");
#endif

#endif
