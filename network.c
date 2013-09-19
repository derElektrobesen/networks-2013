#include "network.h"

int init_server(struct sockaddr_in *addr, int queue_len) {
    int sock = -1;
    int reuse = 1;
    int err;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[Server] socket failure");
        err = errno;
        goto errout;
        //return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        perror("[Sever] setsockopt failure\n");
        err = errno;
        goto errout;        
        //return -1;
    }
    
    log("Server created: %d\n", sock);

    if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("[Server] bind failure");
        err = errno;
        goto errout;        
        //return -1;
    }

    if (listen(sock, queue_len) < 0) { 
        perror("[Server] listen failure");
        err = errno;
        goto errout;        
        //return -1;
    }

    return sock;
errout:
    close(sock);    
    errno = err;
    return -1;
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
                log("Connection closed: %d\n", *(opened_sockets + i));
                close(*(opened_sockets + i));
                (*max_index)--;
                offset++;
                i--;
            } else {
                /* TODO: Fix string cutting */
                buf[bytes_read - 1] = 0;
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
                perror("Accept failure!\n");
                continue;
            }
            log("Accepted connection: %d\n", cli_sock);
            make_sock_nonblock(cli_sock);
            if (sockets_count > MAX_CONNECTIONS - 1) {
                log("Connection refused: too many connections\n");
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
    char *message = "Message recieved!\n";
    log("Message recieved from socket %d: %s\n", sender_sock, msg);
    send(sender_sock, message, strlen(message), 0);
    return 0;
}

int main() {
    create_server(process_message);
    return 0;
}
