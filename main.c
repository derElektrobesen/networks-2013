#include "network.h"

#ifdef CLI
#include <time.h>

#   include "cli.h"

struct sockets_queue q;

void start(int sig) {
    recieve_file("file", (const unsigned char *)"3dfec437ab061d838bbbc4ca1fd2dfcf", 10, &q);
}

int main(int argc, char **argv) {

#ifdef DONT_DO_SRAND
    srand(1);
#else
    srand(time(NULL));
#endif
    signal(SIGUSR1, &start);
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
