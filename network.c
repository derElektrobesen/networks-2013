#include "network.h"

static int init_server_err(int sock, int err_no) {
    if (sock > 0)
        close(sock);
    errno = err_no;
    return -1;
}

int init_server(struct sockaddr_in *addr, int queue_len) {
    int sock = -1;
    int reuse = 1;
    int err_no;

    sock = socket(AF_INET, SOCK_STREAM, 0);
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
    
    log(SERVER, "Server created: %d", sock);

    if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        err_n(SERVER, "bind failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }

    if (listen(sock, queue_len) < 0) { 
        err_n(SERVER, "listen failure");
        err_no = errno;
        return init_server_err(sock, err_no);
    }

    return sock;
}

static int init_client_err(int sock, int err_no) {
    if (sock)
        close(sock);
    errno = err_no;
    return -1;
}

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen) {
    while (1) {
        log(CLIENT, "Trying to connect...");
		if (connect(sockfd, addr, alen) == 0) {
            log(CLIENT, "Connection successfull.");
			return 0;
        }
        log(CLIENT, "Connectin failure.");
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
    
    if (connect_retry(sock, ailist->ai_addr, ailist->ai_addrlen) < 0) {
        err_no = errno;
        err_n(CLIENT, "connection failure\n");
        return init_client_err(sock, err_no);
    } 
    freeaddrinfo(ailist);
    return sock;
}

int make_sock_nonblock(int sock) {
    int fcntl_flags;
    fcntl(sock, F_GETFL, &fcntl_flags);
    fcntl_flags |= O_NONBLOCK;
    fcntl(sock, F_SETFL, fcntl_flags);
    return 0;
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
                log(CLIENT, "Connection closed: %d", *(opened_sockets + i));
                close(*(opened_sockets + i));
                (*max_index)--;
                offset++;
                i--;
            } else {
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
    char *err_str = "Connection refused!\n";

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
                err_n(SERVER, "Accept failure!");
                continue;
            }
            log(SERVER, "Accepted connection: %d", cli_sock);
            make_sock_nonblock(cli_sock);
            if (sockets_count > MAX_CONNECTIONS - 1) {
                log(SERVER, "Connection refused: too many connections");
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

int create_server(socket_callback callback) {
    int srv_sock = -1;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = init_server(&addr, SOMAXCONN);
    if (srv_sock == -1)
        return -1;

    recieve_messages(srv_sock, callback);
    close(srv_sock);
    return 0;
}

int process_message(int sender_sock, const char *msg, ssize_t count) {
    char *message = "Message recieved!";
    log(CLIENT, "Message recieved from socket %d: %s", sender_sock, msg);
    send(sender_sock, message, strlen(message), 0);
    return 0;
}

int main(int argc, char *argv[]) {
#ifdef SRV
    create_server(process_message);
#elif defined CLI
    char *msg = "Bakit zadrot";
    if (argc >= 2) {
        int s;
        s = create_client(argv[1]);
        send(s, msg, strlen(msg), 0);
    } else
        err(-1, "Address is required");
#else
    err("Compile define option is required");
#endif
    return 0;
}
