#include "network.h"

#ifdef CLI
static const char *message = "Bakit zadrot!";

/* Функция рассылающая сообщения серверам в локальной сети */
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

        sleep(SHORT_TIMEOUT);
    }

    return 0;
}

/*
 * Callback-функция которая вызывается при получении сообщения клиентом. 
 * Обрабатывает сообщение полученное от сервера.
 */
int process_srv_message(int sock, const char *msg, ssize_t len) {
    /* TODO: make server messages processing */
    log(CLIENT, "Recieved from server %d: %s", sock, msg);
    return 0;
}
#endif

#ifdef SRV

/*
 * Callback-функция которая вызывается при получении сообщения сервером.
 * Обрабатывает сообщение полученное от клиента
 */
int process_message(int sender_sock, const char *msg, ssize_t count) {  
    char *message = "Message recieved!\n";
    log(CLIENT, "message recieved from socket %d: %s", sender_sock, msg);
    send(sender_sock, message, strlen(message), 0);
    return 0;
}
#endif

int main(int argc, char *argv[]) {
    int result = 0;
#ifdef SRV
    result = start_server(&process_message);
#elif defined CLI
    result = start_client(&process_srv_message, &process_servers);
#else
    err(-1, "Compile define option is required");
    result = 1;
#endif
    return result;
}
