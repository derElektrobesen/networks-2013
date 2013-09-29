#include "network.h"

static int init_server_err(int sock, int err_no) {
    if (sock > 0)
        close(sock);
    errno = err_no;
    return -1;
}

int init_server(struct sockaddr_in *addr, int queue_len, 
        int proto) {
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

int make_sock_nonblock(int sock) {
    int e;
    e = fcntl(sock, F_SETFL, O_NONBLOCK);
    if (e < 0) 
        err_n(-1, "fcntl failure");
    return e;
}

static int init_client_err(int sock, int err_no) {
    if (sock)
        close(sock);
    errno = err_no;
    return -1;
}

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen, int max_try) {
    int i = 0;
    while (i < max_try) {
        i++;
        log(CLIENT, "trying to connect...");
		if (connect(sockfd, addr, alen) == 0) {
            log(CLIENT, "connection successfull.");
			return 0;
        }
        log(CLIENT, "connection failure.");
        sleep(RETRY_TIMEOUT);
	}
	return -1;
}

int create_client(const char *addr) {
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

int process_sockets(fd_set *set, socket_callback callback, 
        int *opened_sockets, int *max_index) {
    int i;
    ssize_t bytes_read;
    char buf[BUF_MAX_LEN];
    int offset;
    int max_sock = -1;

    for (i = 0, offset = 0; i < *max_index; ) {
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
        i++;
    }
    return max_sock;
}

int recieve_messages(int sock, socket_callback callback) {
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

inline int check_detected_conn(const char *msg, size_t len) {
    return strncmp(msg, IDENT_MSG, len);
}
inline int prepare_broadcast_msg(char *msg, int max_len) {
    return snprintf(msg, max_len, "%s", IDENT_MSG); 
}

int wait_connection(struct sockaddr_in *addr, int srv_sock) {
    char buf[BUF_MAX_LEN];
    int flag = 0;
    socklen_t addr_len = sizeof *addr;

    log(CLIENT, "waiting connection");
    while (!flag) {
        if (recvfrom(srv_sock, buf, BUF_MAX_LEN, 0, (struct sockaddr *)addr, &addr_len) > 0) {
            log(BROADCAST, "detected connection with message: %s", buf);
            if (check_detected_conn(buf, BUF_MAX_LEN) == 0) 
                flag = 1;
        }
    }
    return 0;
}

void *broadcast_start(void *arg) {
    int sock;
    struct sockaddr_in brc_addr;
    int brc_perms;
    int msg_len;
    char *brc_ip = "255.255.255.255";
    char msg[BUF_MAX_LEN];

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        err_n(BROADCAST, "socket failure");
        return NULL;
    }

    brc_perms = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&brc_perms, sizeof(brc_perms)) < 0) {
        err_n(BROADCAST, "setsockopt failure");
        return NULL;
    }

    msg_len = prepare_broadcast_msg(msg, BUF_MAX_LEN);

    memset(&brc_addr, 0, sizeof(brc_addr));
    brc_addr.sin_family = AF_INET;
    brc_addr.sin_addr.s_addr = inet_addr(brc_ip);
    brc_addr.sin_port = htons(PORT);

    while (1) {
        log(BROADCAST, "sending broadcast message");
        if (sendto(sock, msg, msg_len, 0, (struct sockaddr *)&brc_addr, sizeof(brc_addr)) != msg_len)
            err_n(BROADCAST, "sendto failure");
        sleep(RETRY_TIMEOUT);
    }
    return NULL;
}

inline int broadcast(pthread_t *thread) {
    int err = 0;

    err = pthread_create(thread, NULL, &broadcast_start, NULL);
    if (err != 0)
        err_n(BROADCAST, "pthread_create failure");
    else
        log(BROADCAST, "broadcast thread created successfully");

    return err;
}

int create_server(socket_callback callback) {
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

int process_message(int sender_sock, const char *msg, ssize_t count) {
    char *message = "Message recieved!\n";
    log(CLIENT, "message recieved from socket %d: %s", sender_sock, msg);
    send(sender_sock, message, strlen(message), 0);
    return 0;
}

int accept_conn(struct sockets_queue *q, struct sockaddr_in *srv_addr) {
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

void *wait_servers(void *arg) {
    struct sockaddr_in srv_addr;
    struct sockaddr_in sock_addr;
    struct sockets_queue *q = (struct sockets_queue *)arg;
    int i, flag;
    int srv_sock;
    int rc;

    log(CLIENT, "client broadcast thread created");

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = init_server(&sock_addr, SOMAXCONN, SOCK_DGRAM);
    if (srv_sock == -1)
        return NULL;

    while (1) {
        wait_connection(&srv_addr, srv_sock);

        rc = pthread_rwlock_rdlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_rdlock");

        for (i = 0, flag = 0; !flag && i < q->count; i++) {
            if (srv_addr.sin_addr.s_addr == q->addrs[i])
                flag = 1;
        }

        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");

        if (flag == 0)
            accept_conn(q, &srv_addr);
        else
            log(BROADCAST, "connection is already established: pass");
    }
    close(srv_sock);
    return NULL;
}

int process_srv_message(int sock, const char *msg, ssize_t len) {
    /* TODO: make server messages processing */
    log(CLIENT, "Recieved from server %d: %s", sock, msg);
    return 0;
}

int recv_srv_msg(fd_set *set, struct sockets_queue *q) {
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
                log(CLIENT, "server %d had been disconnected", q->sockets[i]);
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
                process_srv_message(q->sockets[i], msg, bytes_read);
            }
        }
    }
    if (locked) {
        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
    }
    return 0;
}

void *recieve_servers_messages(void *arg) {
    fd_set set;
    int rc, i;
    int max_sock_fd;
    struct sockets_queue *q = (struct sockets_queue *)arg;

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
        if (q->count == 0) {
            rc = pthread_rwlock_unlock(&(q->rwlock));
            check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
            sleep(RETRY_TIMEOUT);
        } else {
            rc = pthread_rwlock_unlock(&(q->rwlock));
            check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");
            if (select(max_sock_fd + 1, &set, NULL, NULL, NULL) > 0) {
                recv_srv_msg(&set, q);
            }
        }
    }
    return NULL;
}

int process_servers(struct sockets_queue *q) {
    int rc, i;

    /* TODO: Remove dummy actions */

    while (1) {
        rc = pthread_rwlock_rdlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_rdlock");

        for (i = 0; i < q->count; i++) {
            send(q->sockets[i], message, strlen(message), 0);
        }

        rc = pthread_rwlock_unlock(&(q->rwlock));
        check_rwlock(CLIENT, rc, "pthread_rwlock_unlock");

        sleep(RETRY_TIMEOUT);
    }

    return 0;
}

int start_server() {
    pthread_t brc_thread;
    int r = 0;
    r = broadcast(&brc_thread);
    if (r == 0)
        r = create_server(process_message);
    return r;
}

int start_client() {
    pthread_t brc_thread, srv_thread;
    struct sockets_queue q = { .rwlock = PTHREAD_RWLOCK_INITIALIZER };
    int err;

    err = pthread_create(&brc_thread, NULL, &wait_servers, &q);
    if (err != 0)
        err_n(CLIENT, "pthread_create failure");

    err = pthread_create(&srv_thread, NULL, &recieve_servers_messages, &q);
    if (err != 0)
        err_n(CLIENT, "pthread_create failure");

    process_servers(&q);
    pthread_rwlock_destroy(&(q.rwlock));
    return 0;
}

int main(int argc, char *argv[]) {
    int result = 0;
#ifdef SRV
    result = start_server();
#elif defined CLI
    result = start_client();
#else
    err(-1, "Compile define option is required");
    result = 1;
#endif
    return result;
}
