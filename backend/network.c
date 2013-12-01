#include "../include/network.h"

static struct message msgs[MAX_CONNECTIONS];
static struct gui_actions *g_acts;

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
    int buf_len = BUF_MAX_LEN;

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
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_len, sizeof(int)) < 0) {
        err_n(SERVER, "setsockopt failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }
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
 * Ф-ия ожидает получения всего куска данных
 * Возвращает число считанных байт если передача завершена, -1 в случае ошибки
 * и 0 если еще не все сообщение было получено
 */
static ssize_t receive_data(int sock, char *buf, size_t len) {
    ssize_t rlen = 0;
    char size[MSG_LEN_T_SIZE];
    struct message *msg = msgs + sock;
    
    if (sizeof(size_t) != MSG_LEN_T_SIZE)
        err(OTHER, "Data len size != %d bytes", MSG_LEN_T_SIZE);

    if (msg->bytes_count == 0) {
        if (recv(sock, size, sizeof(size), MSG_WAITALL) != sizeof(size)) {
            rlen = -1;
        } else {
            memcpy(&(msg->bytes_count), size, sizeof(rlen) < MSG_LEN_T_SIZE ? sizeof(rlen) : MSG_LEN_T_SIZE);
            log(OTHER, ">>> receive_data, length = %lu, sock = %d", msg->bytes_count, sock);
            msg->bytes_read = 0;
        }
    } else {
        rlen = recv(sock, buf, msg->bytes_count - msg->bytes_read, 0);
        if (!rlen) {
            rlen = -1;
            err_n(OTHER, "recv data failure");
            msgs[sock].bytes_count = 0;
        } else {
            memcpy(msg->message + msg->bytes_read, buf, rlen);
            msg->bytes_read += rlen;
            if (msg->bytes_read == msg->bytes_count) {
                rlen = msg->bytes_count;
                msg->bytes_count = 0;
                memcpy(buf, msg->message, len > rlen ? rlen : len);
            } else
                rlen = 0;
        }
    }
    return rlen;
}

/**
 * Ф-ия посылает данные сокету
 */
ssize_t send_data(int sock, char *buf, size_t len, int flags) {
    char size[MSG_LEN_T_SIZE];
    ssize_t r = sizeof(len) < MSG_LEN_T_SIZE ? sizeof(len) : MSG_LEN_T_SIZE;

    if (sizeof(size_t) != MSG_LEN_T_SIZE)
        err(OTHER, "Data len size != %d bytes", MSG_LEN_T_SIZE);

    memcpy(size, &len, r);
    log(OTHER, "<<< send_data, length = %lu, sock = %d", len, sock);

    if (send(sock, size, MSG_LEN_T_SIZE, flags) != r) {
        r = -1;
        err_n(OTHER, "send data size failure");
    } else if (send(sock, buf, len, flags) != len) {
        r = -1;
        err_n(OTHER, "send data failure");
    } else
        r = 0;

    return len;
}

/**
 * Ф-ия ищет в списке names элемент what и если он был найден, возвращает
 * его. Если элемент не был найден, возвращает NULL
 */
inline static json_char *search_option(json_char *what, json_char **names, json_char **values, unsigned int count) {
    unsigned int i;
    json_char *r = NULL;

    for (i = 0; !r && i < count; i++)
        if (strcmp(what, names[i]) == 0)
            r = values[i];
    return r;
}

/**
 * Ф-ия извлекает строку из переданного ей элемента json структуры
 */
inline static json_char *get_string_value(json_value *cur) {
    json_char *r = NULL;
    if (cur->type == json_string)
        r = cur->u.string.ptr;
    return r;
}

/**
 * Ф-ия обрабатывает переданные ей opts в зависимости от события act и вополняет
 * соответствующие действия
 */
static void gui_actions_dispatcher(json_char *act, json_char **onames,
        json_char **opts, unsigned int ocount, const struct sockets_queue *q) {
    if (g_acts->start_trm && strcmp(act, START_TRM_ACT) == 0)
        g_acts->start_trm(onames, opts, ocount, q);
    else if (g_acts->stop_trm && strcmp(act, STOP_TRM_ACT) == 0)
        g_acts->stop_trm(onames, opts, ocount, q);
    else if (g_acts->terminate && strcmp(act, TERMINATE_ACT) == 0)
        g_acts->terminate(onames, opts, ocount, q);
    else
        err(OTHER, "unknown action given: %s", act);
}

/**
 * Ф-ия обрабатывает пришедший json и выполняет соответствующие ему действия
 */
static void process_gui_message(int sock, const struct sockets_queue *q) {
    json_value *json;
    json_char *cur;
    json_char data[BUF_MAX_LEN];
    json_char *names[JSON_MAX_OPTS];
    json_char *values[JSON_MAX_OPTS];
    unsigned int count = 0;
    ssize_t data_len;
    unsigned int i;

    do {
        data_len = receive_data(sock, data, st_arr_len(data));
    } while (!data_len);

    json = json_parse(data, data_len);

    if (json->type == json_object) {
        for (i = 0; i < json->u.object.length; i++) {
            names[count] = (json->u.object.values + i)->name;
            values[count] = get_string_value((json->u.object.values + i)->value);
            if (!values[count])
                err(OTHER, "JSON parsing failure on option %s", names[count]);
            else
                count++;
        }
    }

    cur = search_option("action", names, values, count);
    if (!cur)
        err(OTHER, "Invalid JSON came: 'type' field is required");
    else
        gui_actions_dispatcher(cur, names, values, count, q);

    json_value_free(json);
}

/**
 * Ф-ия инициализирует сокет для общения с GUI и возвращает
 * его дескриптор
 */
static int init_gui_sock(const char *sock_path) {
    struct sockaddr_un addr;
    int sock;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sock_path);

    if (connect(sock, (struct sockaddr *)(&addr), sizeof(addr))) {
        err_n(OTHER, "connect failure");
        sock = -1;
    }
    if (g_acts)
        g_acts->sock = sock;

    return sock;
}

/**
 * Функция устанавливает режим
 * неблокирующего ввода\вывода для сокета.
 */
static int make_sock_nonblock(int sock) {
    int e = 0;
    /* e = fcntl(sock, F_SETFL, O_NONBLOCK); */
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
    int r = -1;
    while (r < 0 && i < max_try) {
        i++;
        log(CLIENT, "connecting...");
		if (connect(sockfd, addr, alen) == 0) {
            log(CLIENT, "connection successfull.");
            r = 0;
        } else
            sleep(SHORT_TIMEOUT);
	}
	return r;
}

/**
 * Функция инициализирует клиента и пытается соединится с сервером.
 * В случае удачи возвращает дескриптор сокета.
 */
static int create_client(const char *addr) {
    int sock;
    int err_no;
    int buf_len = BUF_MAX_LEN;
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
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_len, sizeof(int)) < 0) {
        err_n(SERVER, "setsockopt failure");
        err_no = errno;
        return init_server_err(sock, err_no);
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

    char *g_opts[] = {"id"};
    char *g_vals[] = {buf};
    int count = 1;

    for (i = 0, offset = 0; i < *max_index; i++) {
        if (offset)
            *(opened_sockets + i) = *(opened_sockets + i + offset);
        if (max_sock < *(opened_sockets + i))
            max_sock = *(opened_sockets + i);
        if (FD_ISSET(*(opened_sockets + i), set)) {
            bytes_read = receive_data(*(opened_sockets + i), buf, sizeof(buf));
            if (bytes_read < 0) {
                log(CLIENT, "connection closed: %d", *(opened_sockets + i));
                snprintf(buf, sizeof(buf), "%d", *(opened_sockets + i));
                g_acts->client_removed(g_opts, g_vals, count);
                close(*(opened_sockets + i));
                (*max_index)--;
                offset++;
                i--;
            } else if (bytes_read) {
                log(OTHER, "bytes received: %lu", bytes_read);
                buf[bytes_read] = 0;
                if (callback)
                    callback(*(opened_sockets + i), buf, bytes_read);
            }
        }
    }
    return max_sock;
}

static void get_sock_ip(int sock, char *ip, int max_len) {
    struct sockaddr_storage addr;
    struct sockaddr_in *s;
    struct sockaddr_in6 *s6;
    socklen_t len;

    len = sizeof(addr);
    getpeername(sock, (struct sockaddr *)&addr, &len);

    if (addr.ss_family == AF_INET) {    /* IPv4 */
        s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &s->sin_addr, ip, max_len);
    } else {                            /* IPv6 */
        s6 = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &s6->sin6_addr, ip, max_len);
    }
}

/**
 * Функция ожидает новые соединения от клиентов.
 */
static int receive_messages(int sock, socket_callback callback) {
    int cli_sock = -1;
    int sockets[MAX_CONNECTIONS];
    int sockets_count = 0;
    int gui_sock;
    fd_set set;
    int max_sock_fd;
    int i;
    char *err_str = "connection refused!\n";
    char buf[255];
    char ip_addr[INET6_ADDRSTRLEN];

    char *g_opts[] = {"ip", "id"};
    char *g_vals[2] = {ip_addr, buf};
    int count = 2;

    make_sock_nonblock(sock);

    gui_sock = init_gui_sock(INTERFACE_SRV_SOCKET_PATH);
    max_sock_fd = sock > gui_sock ? sock : gui_sock;

    while (1) {
        FD_ZERO(&set);
        FD_SET(sock, &set);
        if (gui_sock >= 0)
            FD_SET(gui_sock, &set);

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
            get_sock_ip(cli_sock, ip_addr, sizeof(ip_addr));
            snprintf(buf, sizeof(buf), "%d", cli_sock);
            g_acts->client_added(g_opts, g_vals, count);
            log(SERVER, "accepted connection: %d (%s)", cli_sock, ip_addr);
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
        }
        if (gui_sock >= 0 && FD_ISSET(gui_sock, &set))
            process_gui_message(gui_sock, NULL);

        max_sock_fd = process_sockets(&set, callback, sockets, &sockets_count);
        if (max_sock_fd < sock)
            max_sock_fd = sock;
        if (max_sock_fd < gui_sock)
            max_sock_fd = gui_sock;
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
        /* log(BROADCAST, "sending broadcast message");  */
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

    log(SERVER, "server created: %d", srv_sock);

    receive_messages(srv_sock, callback);
    close(srv_sock);
    return 0;
}

/**
 * Функция устанавливает соединение.
 * Возвращает номер сокета в случае успешного соединения и меньше нуля иначе
 */
static int accept_conn(struct sockets_queue *q, struct sockaddr_in *srv_addr) {
    char srv_ch_addr[INET_ADDRSTRLEN];
    int s, r = -1;

    char buf[255];
    char *o_names[] = {"ip", "id"};
    char *o_vals[] = {srv_ch_addr, buf};
    unsigned int count = 2;

    if (inet_ntop(AF_INET, &(srv_addr->sin_addr), srv_ch_addr, INET_ADDRSTRLEN)) {
        s = create_client(srv_ch_addr);

        log(BROADCAST, "accepted server: %s", srv_ch_addr);
        snprintf(buf, sizeof(buf), "%d", s);
        g_acts->server_added(o_names, o_vals, count);

        q->addrs[q->count] = srv_addr->sin_addr.s_addr;
        q->sockets[q->count++] = s;
        r = s;
    } else {
        err(BROADCAST, "inet_ntop failure");
        r = -1;
    }
    return r;
}

/**
 * Ф-ия обрабатывает входные broadcast сообщения.
 * В случае обнаружения нового сервера, добавляет его в список.
 * Возвращает >= 0 в случае успеха,
 *            -1   в случае loopback сервера
 *            -2   в случае, если соединение уже установлено
 *            -3   в случае других ошибок
 */
static int process_broadcast_servers(int sock, struct sockets_queue *q) {
    char buf[BUF_MAX_LEN];
    int flag = 0;
    int i;

#ifndef USE_LOOPBACK
    int ips_count = 0;
    uint32_t local_ips[MAX_INTERFACES_COUNT];
#endif

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (recvfrom(sock, buf, BUF_MAX_LEN, 0, (struct sockaddr *)(&addr), &addr_len) > 0) {
#ifndef USE_LOOPBACK
        /** Check detected connection for loopback */
        ips_count = get_hostIPs(local_ips, MAX_INTERFACES_COUNT, 0);
        for (i = 0; !flag && i < ips_count; i++)
            if (local_ips[i] == addr.sin_addr.s_addr)
                flag = 1;
        if (!flag)
#endif
        if (check_detected_conn(buf, BUF_MAX_LEN) == 0) {
            /** Connection is ok. Check for already established conn */
            for (i = 0; !flag && i < q->count; i++)
                if (addr.sin_addr.s_addr == q->addrs[i])
                    flag = 2;
            if (!flag) {
                flag = accept_conn(q, &addr);
                if (flag < 0)
                    flag = 3; /** Another errors */
            }
        }
    } else
        flag = 3;
    return flag;
}

/**
 * Функция производит обработку сокета который готов к чтению.
 * Вызывается callback-функция для обработки сообщения.
 */
static int recv_srv_msg(fd_set *set, struct sockets_queue *q, socket_callback callback) {
    ssize_t bytes_read;
    char msg[BUF_MAX_LEN];
    int i, offset;
    char buf[255];

    char *o_names[] = {"id"};
    char *o_vals[] = {buf};
    int count = 1;

    for (i = 0, offset = 0; i < q->count; i++) {
        if (offset) {
            q->sockets[i] = q->sockets[i + offset];
            q->addrs[i] = q->addrs[i + offset];
        }
        if (FD_ISSET(q->sockets[i], set)) {
            bytes_read = receive_data(q->sockets[i], msg, sizeof(msg));
            if (bytes_read < 0) {
                snprintf(buf, sizeof(buf), "%d", q->sockets[i]);
                g_acts->server_removed(o_names, o_vals, count);
                log(CLIENT, "server %d has been disconnected", q->sockets[i]);
                offset++;
                i--;
                close(q->sockets[i]);
                q->count--;
            } else if (bytes_read) {
                log(OTHER, "bytes read: %lu", bytes_read);
                msg[bytes_read] = 0;
                if (callback)
                    callback(q->sockets[i], msg, bytes_read);
            }
        }
    }
    return 0;
}

static void send_gui_message(char *act, char **opts_names, char **opts_vals, unsigned int count) {
    static char msg[BUF_MAX_LEN];
    size_t len;
    int i;

    static char *g_opts[JSON_MAX_OPTS];
    static char *g_vals[JSON_MAX_OPTS];

    if (g_acts->sock >= 0) {
        count = count >= JSON_MAX_OPTS ? JSON_MAX_OPTS - 1 : count;
        for (i = 0; i < count; i++) {
            g_opts[i] = opts_names[i];
            g_vals[i] = opts_vals[i];
        }
        g_opts[count] = "action";
        g_vals[count] = act;

        len = json_print(g_opts, g_vals, count + 1, msg, sizeof(msg));
        send_data(g_acts->sock, msg, len, 0);
    }
}

/**
 * Функция обрабатывает сообщения полученные от серверов
 * с которыми клиент установил соединение.
 * Если соединений нет, то процесс засыпает на некоторое время.
 */
static int receive_servers_messages(
        socket_callback process_srv_msg_callback,
        queue_dispatcher dispatcher,
        struct sockets_queue *q) {
    fd_set set;
    int i;
    int max_sock_fd;
    int broadcast_sock;
    int gui_sock;
    struct sockaddr_in broadcast_sock_addr;
    struct timeval timeout = {
#ifdef ALARM_S_DELAY
        .tv_sec = ALARM_S_DELAY,
#else
        .tv_sec = 0,
#endif
#ifdef ALARM_U_DELAY
        .tv_usec = ALARM_U_DELAY,
#else
        .tv_usec = 0,
#endif
    };
    int new_socks[MAX_CONNECTIONS];
    int new_socks_count = 0;
    int new_sock;

    gui_sock = init_gui_sock(INTERFACE_CLI_SOCKET_PATH);

    broadcast_sock_addr.sin_family = AF_INET;
    broadcast_sock_addr.sin_port = htons(PORT);
    broadcast_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    broadcast_sock = init_server(&broadcast_sock_addr, SOMAXCONN, SOCK_DGRAM);
    if (broadcast_sock == -1)
        return 1;

    log(CLIENT, "client created");

    while (1) {
        FD_ZERO(&set);
        FD_SET(broadcast_sock, &set);
        if (gui_sock >= 0)
            FD_SET(gui_sock, &set);

        max_sock_fd = broadcast_sock > gui_sock ? broadcast_sock : gui_sock;

        for (i = 0; i < q->count; i++) {
            FD_SET(q->sockets[i], &set);
            if (max_sock_fd < q->sockets[i])
                max_sock_fd = q->sockets[i];
        }

        if (select(max_sock_fd + 1, &set, NULL, NULL, dispatcher ? &timeout : NULL) > 0) {
            if (FD_ISSET(broadcast_sock, &set)) {
                new_sock = process_broadcast_servers(broadcast_sock, q);
                if (new_sock >= 0)
                    new_socks[new_socks_count++] = new_sock;
            }
            if (gui_sock >= 0 && FD_ISSET(gui_sock, &set))
                process_gui_message(gui_sock, q);
            recv_srv_msg(&set, q, process_srv_msg_callback);
        }

        if (dispatcher && timeout.tv_sec == 0 && timeout.tv_usec == 0) {
            /* Timeout came */
#ifdef ALARM_S_DELAY
            timeout.tv_sec = ALARM_S_DELAY;
#endif
#ifdef ALARM_U_DELAY
            timeout.tv_usec = ALARM_U_DELAY;
#endif
            dispatcher(new_socks, new_socks_count);
            new_socks_count = 0;
        }
    }
    return 0;
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
 */
int start_client(socket_callback process_srv_msg_callback,
        queue_dispatcher dispatcher, struct sockets_queue *q) {
    q->count = 0;
    return receive_servers_messages(process_srv_msg_callback, dispatcher, q);
}

#define define_act(name, macro) \
inline static void g_##name(char **o, char **v, unsigned int c) { \
    return send_gui_message((macro), o, v, c); \
}

define_act(package_sent, PACKAGE_SENT_ACT);
define_act(package_received, PACKAGE_RECIEVED_ACT);
define_act(server_added, SERVER_ADDED_ACT);
define_act(client_added, CLIENT_ADDED_ACT);
define_act(server_removed, SERVER_REMOVED_ACT);
define_act(client_removed, CLIENT_REMOVED_ACT);
define_act(answer, ANSWER_ACT);
define_act(file_received, FILE_RECEIVED_ACT);

#undef define_act

/**
 * Ф-ия устанавливает обработчики которые будут вызваны при посылке сообщения гую
 * клиентом или сервером
 */
void setup_gui_msgs(struct gui_actions *acts) {
    g_acts = acts;
    g_acts->package_sent = &g_package_sent;
    g_acts->package_received = &g_package_received;
    g_acts->server_added = &g_server_added;
    g_acts->client_added = &g_client_added;
    g_acts->server_removed = &g_server_removed;
    g_acts->client_removed = &g_client_removed;
    g_acts->answer = &g_answer;
    g_acts->file_received = &g_file_received;
}
