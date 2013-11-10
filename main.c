#include "network.h"

#ifdef CLI
#include <time.h>

#   include "cli.h"

struct sockets_queue q;

void start(int sig) {
    //recieve_file("file", (const unsigned char *)"\x3d\xfe\xc4\x37\xab\x06\x1d\x83\x8b\xbb\xc4\xca\x1f\xd2\xdf\xcf", 10, &q);
    recieve_file("RobertLove.pdf", (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e", 3244909, &q);
}

int main(int argc, char **argv) {

#ifdef DONT_DO_SRAND
    srand(1);
#else
    srand(time(NULL));
#endif
    signal(SIGUSR1, &start);
    log(CLIENT, "Current pid: %d", getpid());
    return start_client(&process_srv_message, &main_dispatcher, &q);
}



#elif defined SRV
#   include "srv.h"

int main(int argc, char **argv) {
    return start_server(&process_client_message);
}

#else

int main(int argc, char **argv) {
    err(-1, "Compile define option is required");
    return 1;
}

#endif
