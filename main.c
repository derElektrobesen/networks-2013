#include "network.h"

#ifdef CLI
#include <time.h>

#   include "cli.h"

int main(int argc, char **argv) {
    struct sockets_queue q;

#ifdef DONT_DO_SRAND
    srand(1);
#else
    srand(time(NULL));
#endif

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
