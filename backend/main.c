#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../include/network.h"

#ifdef DAEMONIZE

#   define LOCK_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

inline static int lockfile(int fd) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}

static int is_running(const char *file_name) {
    int fd;
    char buf[16];
    fd = open(file_name, O_RDWR | O_CREAT, LOCK_MODE);
    if (fd < 0) {
        syslog(LOG_ERR, "could not open file %s", file_name);
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return -1;
        }
        syslog(LOG_ERR, "could not open file %s", file_name);
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return fd;
}

static void daemonize(const char *daemon_name) {
    char file_name[255];
    int i;
    pid_t pid;
    struct rlimit rl;
    int block_fd;

    snprintf(file_name, st_arr_len(file_name), "%s/%s.pid", LOCK_FILE_PATH, daemon_name);

    if ((block_fd = is_running(file_name)) == -1) {
        syslog(LOG_ERR, "%s is already running", daemon_name);
        exit(1);
    }

    umask(0);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0){
        /* add log */
        return;
    }
    
    if ((pid = fork()) < 0)
        return;
    else if (pid != 0)
        exit(0);
    setsid();

    if (chdir("/") < 0) {
        /*add log*/
        return;
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max && i != block_fd; i++)
        close(i);
   
    openlog(daemon_name, LOG_CONS | LOG_PID, LOG_DAEMON);
    /* TODO: syslog -> macro.h */
}
#endif /* DAEMONIZE */

#ifdef CLI
#   include "../include/cli.h"
    struct sockets_queue q;
#elif defined SRV
#   include "../include/srv.h"
#else
#   error "CLI or SRV macro definition is required"
#endif

int main(int argc, char **argv) {
    struct gui_actions acts;
    memset(&acts, 0, sizeof(acts));

    if (sizeof(struct cli_fields) + PIECE_LEN_TSIZE != PROTO_STRUCT_SIZE) {
        err(OTHER, "Wrong PROTO_STRUCT_SIZE given, %zu expected",
                sizeof(struct cli_fields) - PIECE_LEN_TSIZE);
        exit(1);
    }

#ifdef DONT_DO_SRAND
    srand(1);
#else
    srand(time(NULL));
#endif /* DONT_DO_SRAND */
#ifdef DAEMONIZE
	daemonize(argv[0]);
#endif /* DAEMONIZE */

    setup_gui_acts(&acts);  /* Установить и сохранить обработчики, вызываемые при получении сообщения   */
    setup_gui_msgs(&acts);  /* Установить и сохранить обработчики, вызываемые при посылке сообщения     */

#ifdef CLI
    return start_client(&process_srv_message, &main_dispatcher, &q);
#else
    return start_server(&process_client_message);
#endif /* CLI */
}
