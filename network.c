#include "network.h"

/**
 * Функция возвращает ошибку, если
 * инициализация сервера закончилась неудачно.
 */
static int init_server_err(int sock, int err_no) {
    if (sock > 0)
        close(sock);
    errno = err_no;
    return -1;
}

/**
 * Функция инициализирует сервер.
 * Создает сокет, связывает его с адресом и
 * устанавливает длину очереди для обработки.
 * Возвращает дескриптор этого сокета.
 */
static int init_server(struct sockaddr_in *addr, int queue_len, int proto) {
    int sock = -1;
    int reuse = 1;
    int err_no;

    sock = socket(AF_INET, proto, 0);
    if (sock < 0) {
        err_n(SERVER, "socket failure");
        err_no = errno;
        return init_server_err(-1, err_no);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        err_n(SERVER, "setsockopt failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }

    log(SERVER, "server created: %d", sock);

    if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        err_n(SERVER, "bind failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }

    if (proto == SOCK_STREAM && listen(sock, queue_len) < 0) {
        err_n(SERVER, "listen failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }

    return sock;
}

/**
 * Функция устанавливает режим
 * неблокирующего ввода\вывода для сокета.
 */
static int make_sock_nonblock(int sock) {
    int e;
    e = fcntl(sock, F_SETFL, O_NONBLOCK);
    if (e < 0)
        err_n(-1, "fcntl failure");
    return e;
}

/**
 * Функция возвращает ошибку, если
 * инициализация клиента закончилась неудачно.
 */
static int init_client_err(int sock, int err_no) {
    if (sock)
        close(sock);
    errno = err_no;
    return -1;
}

/**
 * Функция пытается установить соединение
 * с сервером по определенному алгоритму.
 * Используется клиентом.
 */
static int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen, int max_try) {
    int i = 0;
    while (i < max_try) {
        i++;
        log(CLIENT, "connecting...");
		if (connect(sockfd, addr, alen) == 0) {
            log(CLIENT, "connection successfull.");
			return 0;
        }
        log(CLIENT, "connection failure.");
        sleep(SHORT_TIMEOUT);
	}
	return -1;
}

/**
 * Функция инициализирует клиента и пытается соединится с сервером.
 * В случае удачи возвращает дескриптор сокета.
 */
static int create_client(const char *addr) {
    int sock;
    int err_no;
    struct addrinfo hint;
    struct addrinfo *ailist;
    char port[4];

    snprintf(port, 5, "%d", PORT);

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    if ((err_no = getaddrinfo(addr, port, &hint, &ailist)) < 0) {
        err_no = errno;
        err_n(CLIENT, "getaddrinfo: %s", gai_strerror(err_no));
        return init_client_err(-1, err_no);
    }

    if ((sock = socket(ailist->ai_family, ailist->ai_socktype, ailist->ai_protocol)) < 0) {
        err_no = errno;
        err_n(CLIENT, "socket failure");
        return init_client_err(-1, err_no);
    }

    make_sock_nonblock(sock);

    if (connect_retry(sock, ailist->ai_addr, ailist->ai_addrlen, 5) < 0) {
        err_no = errno;
        err_n(CLIENT, "connection failure\n");
        return init_client_err(sock, err_no);
    }
    freeaddrinfo(ailist);
    return sock;
}

/**
 * Получает сообщение от сокета, на котором возникло некоторое событие
 * и обрабатывает его. Удаляет сокет из массива в случае разрыва соединения.
 */
static int process_sockets(fd_set *set, socket_callback callback,
        int *opened_sockets, int *max_index) {
    int i;
    ssize_t bytes_read;
    char buf[BUF_MAX_LEN];
    int offset;
    int max_sock = -1;

    for (i = 0, offset = 0; i < *max_index; i++) {
        if (offset)
            *(opened_sockets + i) = *(opened_sockets + i + offset);
        if (max_sock < *(opened_sockets + i))
            max_sock = *(opened_sockets + i);
        if (FD_ISSET(*(opened_sockets + i), set)) {
            bytes_read = recv(*(opened_sockets + i), buf, BUF_MAX_LEN, 0);
            if (bytes_read <= 0) {
                log(CLIENT, "connection closed: %d", *(opened_sockets + i));
                close(*(opened_sockets + i));
                (*max_index)--;
                offset++;
                i--;
            } else {
                if (buf[bytes_read - 1] == '\n')
                    bytes_read--;
                buf[bytes_read] = 0;
                if (callback)
                    callback(*(opened_sockets + i), buf, bytes_read);
            }
        }
    }
    return max_sock;
}

/**
 * Функция ожидает новые соединения от клиентов.
 */
static int recieve_messages(int sock, socket_callback callback) {
    int cli_sock = -1;
    int sockets[MAX_CONNECTIONS];
    int sockets_count = 0;
    fd_set set;
    int max_sock_fd = sock;
    int i;
    char *err_str = "connection refused!\n";

    make_sock_nonblock(sock);

    while (1) {
        FD_ZERO(&set);
        FD_SET(sock, &set);
        for (i = 0; i < sockets_count; i++)
            FD_SET(sockets[i], &set);

        if (select(max_sock_fd + 1, &set, NULL, NULL, NULL) <= 0)
            continue;
        if (FD_ISSET(sock, &set)) {
            /* Присоединился новый клиент, обработаем сокет */
            cli_sock = accept(sock, NULL, NULL);
            if (cli_sock < 0) {
                err_n(SERVER, "accept failure!");
                continue;
            }
            log(SERVER, "accepted connection: %d", cli_sock);
            make_sock_nonblock(cli_sock);
            if (sockets_count > MAX_CONNECTIONS - 1) {
                log(SERVER, "connection refused: too many connections");
                send(cli_sock, err_str, strlen(err_str), 0);
                close(cli_sock);
            } else {
                sockets[sockets_count++] = cli_sock;
                if (cli_sock > max_sock_fd)
                    max_sock_fd = cli_sock;
            }
        } else {
            max_sock_fd = process_sockets(&set, callback, sockets, &sockets_count);
            if (max_sock_fd < sock)
                max_sock_fd = sock;
        }
    }
    return 0;
}

inline static int check_detected_conn(const char *msg, size_t len) {
    return strncmp(msg, IDENT_MSG, len);
}

/* Функция для формирования сообщения для широковещания */
inline static int prepare_broadcast_msg(char *msg, int max_len) {
    return snprintf(msg, max_len, "%s", IDENT_MSG);
}

static int wait_connection(struct sockaddr_in *addr, int srv_sock,
        uint32_t *ignore_ips, int ips_count) {
    char buf[BUF_MAX_LEN];
    int flag = 0;
    int i;
    socklen_t addr_len = sizeof *addr;

    log(CLIENT, "waiting connection");
    while (!flag) {
        if (recvfrom(srv_sock, buf, BUF_MAX_LEN, 0, (struct sockaddr *)addr, &addr_len) > 0) {
            for (i = 0; !flag && i < ips_count; i++)
                if (ignore_ips[i] == addr->sin_addr.s_addr)
                    flag = 1;
            if (flag) {
                flag = 0;
            } else {
                log(BROADCAST, "detected connection with message: %s", buf);
                if (check_detected_conn(buf, BUF_MAX_LEN) == 0)
                    flag = 1;
            }
        }
    }
    return 0;
}

#ifndef USE_LOOPBACK
/**
 * Функция возвращает количество найденных сетевых интерфейсов (IP адреса),
 * адреса найденных интерфейсов помещаются в параметр ips.
 * Параметры:
 * ips - массив IP адресов найденных интерфейсов;
 * max_count - максимальное количество сетевых интерфейсов;
 * use_loopback - ключ использования LOOPBACK (использовать, если значение больше 0).
 */
static int get_hostIPs(uint32_t *ips, int max_count, int use_loopback) {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *addr;
    int i = 0, j;

    if (getifaddrs(&ifap) == 0) {
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if ((ifa->ifa_addr->sa_family == AF_INET) && !(use_loopback && (ifa->ifa_flags & IFF_LOOPBACK))) {
                addr = (struct sockaddr_in *) ifa->ifa_addr;
                for (j = 0; j != -1 && j < i; j++)
                    if (ips[j] == addr->sin_addr.s_addr)
                        j = -1;
                if (j >= 0) {
                    if (i >= max_count) {
                        ifa = NULL;
                        err(BROADCAST, "too many network interfaces found");
                    } else
                        ips[i++] = addr->sin_addr.s_addr;
                }
            }
        }
    }
    return i;
}

/**
 * Функция возвращает количество интерфейсов, которые будут
 * использованы  при широковещании.
 * IP адреса помещаются в параметр ips
 * Параметр msx_count указывает максимальное число сетевых интерфейсов.
 */
static int get_hostIP(char (*ips)[4 * sizeof("000")], int max_count) {
    char ip_addr[4 * sizeof("000")];
    uint32_t ips_i[MAX_INTERFACES_COUNT];
    unsigned char *addr_ref;
    int count;
    int i;

    if (max_count > MAX_INTERFACES_COUNT)
        max_count = MAX_INTERFACES_COUNT;

    count = get_hostIPs(ips_i, max_count, 1);
    for (i = 0; i < count; i++) {
        addr_ref = (unsigned char *)&(ips_i[i]);
        sprintf(ip_addr, "%d.%d.%d.%d",                         // ЧТО ПРОИСХОДИТ ?
                addr_ref[0] & 0xff, addr_ref[1] & 0xff,
                addr_ref[2] & 0xff, 0xff);
        strcpy(ips[i], ip_addr);
        log(BROADCAST, "broadcast ip: %s", ip_addr);
    }
    return count;
}

/**
 * Функция для широковещания.
 * Получает список интерфейсов
 */
static void *broadcast_start(void *arg) {
    int sock;
    int brc_perms;
    int msg_len;
    char msg[BUF_MAX_LEN];
    struct sockaddr_in brc_addrs[MAX_INTERFACES_COUNT];
    char brc_ips[4 * sizeof("000")][MAX_INTERFACES_COUNT];
    int socks[MAX_INTERFACES_COUNT];
    char *brc_ip;
    int ips_count;
    uint32_t ips_i[MAX_INTERFACES_COUNT];
    int ips_i_count;
    int i, j = 0;

    /* Цикл: пока не найдены интерфейсы */
    do {
        ips_count = get_hostIP(brc_ips, MAX_INTERFACES_COUNT);
        if (ips_count == 0) {
            err(BROADCAST, "get_hostIP failure");
            sleep(LONG_TIMEOUT);
        }
    } while (ips_count == 0);

    /* Цикл: для всех найденных интерфейсов призвести широковещание */
    for (i = 0; i < ips_count; i++) {
        brc_ip = brc_ips[i];
        if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
            err_n(BROADCAST, "socket failure");
            return NULL;
        }

        brc_perms = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&brc_perms, sizeof(brc_perms)) < 0) {
            err_n(BROADCAST, "setsockopt failure");
            return NULL;
        }

        memset(&(brc_addrs[i]), 0, sizeof(brc_addrs[i]));
        brc_addrs[i].sin_family = AF_INET;
        brc_addrs[i].sin_addr.s_addr = inet_addr(brc_ip);
        brc_addrs[i].sin_port = htons(PORT);
        socks[i] = sock;
    }

    msg_len = prepare_broadcast_msg(msg, BUF_MAX_LEN);
    while (1) {
        if (j++ % (LONG_TIMEOUT / SHORT_TIMEOUT) == 0) {
            ips_i_count = get_hostIPs(ips_i, MAX_INTERFACES_COUNT, 1);
            if (ips_i_count != ips_count)
                j = -1;
            else
                j = 1;
        }
        for (i = 0; i < ips_count; i++) {
            log(BROADCAST, "sending broadcast message to address %s", brc_ips[i]);
            if (sendto(socks[i], msg, msg_len, 0,
                    (struct sockaddr *)&(brc_addrs[i]), sizeof(brc_addrs[i])) != msg_len) {
                err_n(BROADCAST, "sendto failure");
                j = -1;
            }
        }
        if (j == -1) {
            for (i = 0; i < ips_count; i++)
                close(socks[i]);
            return broadcast_start(arg);
        }
        sleep(SHORT_TIMEOUT);
    }
    return NULL;
}

#else /* USE_LOOPBACK defined */

/**
 * Функция для широковещания, если не требуется
 * игнорировать LOOPBACK.
 * Сообщения посылаемые данным хостом, будут им
 * же и приниматься.
 */
static void *broadcast_start(void *arg) {
    int sock;
    int brc_perms;
    int msg_len;
    char msg[BUF_MAX_LEN];
    struct sockaddr_in brc_addr;
    char *brc_ip = "255.255.255.255";

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        err_n(BROADCAST, "socket failure");
        return NULL;
    }

    brc_perms = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&brc_perms, sizeof(brc_perms)) < 0) {
        err_n(BROADCAST, "setsockopt failure");
        return NULL;
    }
    memset(&brc_addr, 0, sizeof(brc_addr));
    brc_addr.sin_family = AF_INET;
    brc_addr.sin_addr.s_addr = inet_addr(brc_ip);
    brc_addr.sin_port = htons(PORT);

    msg_len = prepare_broadcast_msg(msg, BUF_MAX_LEN);

    while (1) {
        log(BROADCAST, "sending broadcast message");
        if (sendto(sock, msg, msg_len, 0, (struct sockaddr *)&brc_addr, sizeof(brc_addr)) != msg_len)
            err_n(BROADCAST, "sendto failure");
        sleep(SHORT_TIMEOUT);
    }
    return NULL;
}
#endif /* USE_LOOPBACK */

/**
 * Функция создает поток, который при запуске,
 * начинает широковещание.
 * Идентификатор потока записывается в аргумент thread.
 * Если поток не запустился, возвращается код ошибки.
 */
inline static int broadcast(pthread_t *thread) {
    int err = 0;

    err = pthread_create(thread, NULL, &broadcast_start, NULL);
    if (err != 0)
        err_n(BROADCAST, "pthread_create failure");
    else
        log(BROADCAST, "broadcast thread created successfully");

    return err;
}

/**
 * Функция создает и инициализирует сервер.
 * Устанавливает callback-функцию при получении сообщения.
 * Ошибка возвращается, если не удалось инициализировать сервер.
 */
static int create_server(socket_callback callback) {
    int srv_sock = -1;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = init_server(&addr, SOMAXCONN, SOCK_STREAM);
    if (srv_sock == -1)
        return -1;

    recieve_messages(srv_sock, callback);
    close(srv_sock);
    return 0;
}

/**
 * Функция устанавливает соединение.
 */
static int accept_conn(struct sockets_queue *q, struct sockaddr_in *srv_addr) {
    char srv_ch_addr[INET_ADDRSTRLEN];
    int s, rc, r = 0;

    if (inet_ntop(AF_INET, &(srv_addr->sin_addr), srv_ch_addr, INET_ADDRSTRLEN)) {
        log(BROADCAST, "accepted server: %s", srv_ch_addr);
        s = create_client(srv_ch_addr);

        rc = pthread_rwlock_wrlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_wrlock");

        q->addrs[q->count] = srv_addr->sin_addr.s_addr;
        q->sockets[q->count++] = s;

        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
    } else {
        err(BROADCAST, "inet_ntop failure");
        r = 1;
    }
    return r;
}

/**
 * Функция ожидает широковещательного сообщения от сервера.
 */
static void *wait_servers(void *arg) {
    struct sockaddr_in srv_addr;
    struct sockaddr_in sock_addr;
    struct sockets_queue *q = (struct sockets_queue *)arg;
    int i, flag;
    int srv_sock;
    int rc;
    uint32_t local_ips[MAX_INTERFACES_COUNT];
    int ips_count = 0;

    log(CLIENT, "client broadcast thread created");

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = init_server(&sock_addr, SOMAXCONN, SOCK_DGRAM);
    if (srv_sock == -1)
        return NULL;

#ifndef USE_LOOPBACK
    ips_count = get_hostIPs(local_ips, MAX_INTERFACES_COUNT, 0);
#endif
    while (1) {
        flag = 0;
        wait_connection(&srv_addr, srv_sock, local_ips, ips_count);

#ifndef USE_LOOPBACK
        for (i = 0; !flag && i < ips_count; i++) {
            if (srv_addr.sin_addr.s_addr == local_ips[i])
                flag = 2;
        }
        if (!flag) {
        ips_count = get_hostIPs(local_ips, MAX_INTERFACES_COUNT, 0);
#endif
        rc = pthread_rwlock_rdlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_rdlock");

        for (i = 0; !flag && i < q->count; i++) {
            if (srv_addr.sin_addr.s_addr == q->addrs[i])
                flag = 1;
        }

        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");

#ifndef USE_LOOPBACK
        }
#endif
        if (flag == 0)
            accept_conn(q, &srv_addr);
        else if (flag == 1)
            log(BROADCAST, "connection is already established: pass");
    }
    close(srv_sock);
    return NULL;
}

/**
 * Функция производит обработку сокета который готов к чтению.
 * Вызывается callback-функция для обработки сообщения.
 */
static int recv_srv_msg(fd_set *set, struct sockets_queue *q, socket_callback callback) {
    ssize_t bytes_read;
    char msg[BUF_MAX_LEN];
    int i, offset;
    int rc;
    int locked = 0;

    for (i = 0, offset = 0; i < q->count; i++) {
        if (offset) {
            q->sockets[i] = q->sockets[i + offset];
            q->addrs[i] = q->addrs[i + offset];
        }
        if (FD_ISSET(q->sockets[i], set)) {
            bytes_read = recv(q->sockets[i], msg, BUF_MAX_LEN, 0);
            if (bytes_read <= 0) {
                log(CLIENT, "server %d has been disconnected", q->sockets[i]);
                offset++;
                i--;
                locked = 1;
                rc = pthread_rwlock_wrlock(&(q->rwlock));
                check_rwlock(CLIENT, rc, "pthread_rwlock_wrlock");
                close(q->sockets[i]);
                q->count--;
            } else {
                if (msg[bytes_read - 1] == '\n')
                    bytes_read--;
                msg[bytes_read] = 0;
                if (callback)
                    callback(q->sockets[i], msg, bytes_read);
            }
        }
    }
    if (locked) {
        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
    }
    return 0;
}

/**
 * Функция обрабатывает сообщения полученные от серверов
 * с которыми клиент установил соединение.
 * Если соединений нет, то процесс засыпает на некоторое время.
 */
static void *recieve_servers_messages(void *arg) {
    fd_set set;
    int rc, i;
    int max_sock_fd;
    socket_callback callback;
    struct sockets_queue *q;
    void **args = arg;

    callback = (socket_callback)(args[0]);
    q = (struct sockets_queue *)(args[1]);

    log(CLIENT, "server messages reciever thread created");

    while (1) {
        FD_ZERO(&set);
        max_sock_fd = -1;

        rc = pthread_rwlock_rdlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_rdlock");

        for (i = 0; i < q->count; i++) {
            FD_SET(q->sockets[i], &set);
            if (max_sock_fd < q->sockets[i])
                max_sock_fd = q->sockets[i];
        }

        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
        if (q->count == 0)
            sleep(SHORT_TIMEOUT);
        else {
            if (select(max_sock_fd + 1, &set, NULL, NULL, NULL) > 0)
                recv_srv_msg(&set, q, callback);
        }
    }
    return NULL;
}

/**
 * Функция создает сервер.
 * Создается поток, который периодически рассылает
 * широковещательные сообщения по локальной сети.
 * Основной процесс инициализирует сервер и устанавливает
 * callback-функцию.
 */
int start_server(socket_callback process_cli_msg_callback) {
    pthread_t brc_thread;
    int r = 0;
    r = broadcast(&brc_thread);
    if (r == 0)
        r = create_server(process_cli_msg_callback);
    return r;
}

/**
 * Функция создает клиента.
 * Процесс делится на 3 потока, один из которых
 * ожидает подключения серверов.
 * Второй поток получает сообщения от серверов
 * и обрабатывает их.
 */
int start_client(socket_callback process_srv_msg_callback, server_answ_callback srv_answ) {
    pthread_t brc_thread, srv_thread;
    struct sockets_queue q = { .rwlock = PTHREAD_RWLOCK_INITIALIZER };
    int err;
    void *args[] = { process_srv_msg_callback, &q };

    err = pthread_create(&brc_thread, NULL, &wait_servers, &q);
    if (err != 0)
        err_n(CLIENT, "pthread_create failure");

    err = pthread_create(&srv_thread, NULL, &recieve_servers_messages, args);
    if (err != 0)
        err_n(CLIENT, "pthread_create failure");

    if (srv_answ)   /* Обработка ответа сервера */
        srv_answ(&q);
    else
        err(CLIENT, "server answers can't be processed");
    pthread_rwlock_destroy(&(q.rwlock));
    return 0;
}
