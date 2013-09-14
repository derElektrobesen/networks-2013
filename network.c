#include "network.h"

int create_server(socket_callback callback) {
    int srv_sock = -1,
        cli_sock = -1;
    size_t bytes_read = 0;
    int fcntl_flags, flag;
    struct sockaddr_in addr;
    char buf[BUF_MAX_LEN];
    fd_set set;

    srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_sock < 0) {
        perror("Server socket failure!\n");
        return -1;
    }
    log("Server created: %d\n", srv_sock);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(srv_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[Server socket] Bind failure!\n");
        return -1;
    }

    if (listen(srv_sock, SELECT_QUEUE_LEN) < 0) {
        perror("[Server socket] Listen failure!\n");
        return -1;
    }

    fcntl(srv_sock, F_GETFL, &fcntl_flags);
    fcntl_flags |= O_NONBLOCK;
    fcntl(srv_sock, F_SETFL, fcntl_flags);

    FD_ZERO(&set);
    FD_SET(srv_sock, &set);

    while (1) {
        if (select(srv_sock + 1, &set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(srv_sock, &set)) {
                cli_sock = accept(srv_sock, NULL, NULL);
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

int main() {
    create_server(NULL);
    return 0;
}
