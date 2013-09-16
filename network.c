#include "network.h"

int init_server(struct sockaddr_in *addr, int queue_len) {
    int sock = -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Server socket failure");
        return -1;
    }
    log("Server created: %d\n", sock);

    if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("[Server socket] Bind failure");
        return -1;
    }

    if (listen(sock, queue_len) < 0) { 
        perror("[Server socket] Listen failure");
        return -1;
    }

    return sock;
}

int recieve_messages(int sock, socket_callback callback) {
    int cli_sock = -1;
    char buf[BUF_MAX_LEN];
    ssize_t bytes_read;
    int flag;
    fd_set set;

    FD_ZERO(&set);
    FD_SET(sock, &set);

    while (1) {
        if (select(sock + 1, &set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(sock, &set)) {
                cli_sock = accept(sock, NULL, NULL);
                if (cli_sock < 0) {
                    perror("Accept failure!\n");
                    continue;
                }
                log("Accepted connection: %d\n", cli_sock);

                flag = 1;
                while (flag) {
                    bytes_read = recv(cli_sock, buf, BUF_MAX_LEN, 0);
                    if (bytes_read <= 0)
                        flag = 0;
                    else {
                        buf[bytes_read - 1] = 0;
                        log("Server recieved message: %s\n", buf);
                        if (callback)
                            callback(buf, bytes_read);
                    }
                }
                close(cli_sock);
            }
        }
    }
    return 0;
}

int create_server(socket_callback callback) {
    int srv_sock = -1;
    struct sockaddr_in addr;
    int fcntl_flags;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = init_server(&addr, SOMAXCONN);
    if (srv_sock == -1)
        return -1;

    fcntl(srv_sock, F_GETFL, &fcntl_flags);
    fcntl_flags |= O_NONBLOCK;
    fcntl(srv_sock, F_SETFL, fcntl_flags);

    recieve_messages(srv_sock, callback);
    close(srv_sock);
    return 0;
}

int main() {
    create_server(NULL);
    return 0;
}
