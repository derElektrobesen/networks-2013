#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

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
#   include <time.h>
#   include "../include/cli.h"

struct sockets_queue q;

void start(int sig) {
    //receive_file("file", (const unsigned char *)"\x3d\xfe\xc4\x37\xab\x06\x1d\x83\x8b\xbb\xc4\xca\x1f\xd2\xdf\xcf", 10, &q);
    receive_file("RobertLove.pdf", (const unsigned char *)"\x19\x01\x67\x36\x6f\x14\xce\x7e\x9b\x26\x8d\x72\xad\x87\xf1\x48", 3244909, &q);
    //receive_file("data", (const unsigned char *)"\x1d\x0b\xd7\x71\xd2\xa1\xb1\xbf\x4a\x25\xf2\xa4\xfc\x1f\xf6\xf9", 149114, &q);
    //receive_file("data", (const unsigned char *)"\x0d\x0c\x91\xc7\x43\xd9\x1d\x3c\xf2\x4d\x11\xac\x3b\xf3\x9e\x9a", 2047, &q);
    //receive_file("sample", (const unsigned char *)"\xe3\x02\xf9\xec\xd2\xd1\x89\xfa\x80\xaa\xc1\xc3\x39\x2e\x9b\x9c", 27, &q);
    //receive_file("test", (const unsigned char *)"\x19\x33\xb8\x4f\x18\xdd\xb7\x54\x5c\x63\x96\x2b\xe5\xd1\x0b\xb5", 588890, &q);
}

int main(int argc, char **argv) {

#ifdef DONT_DO_SRAND
    srand(1);
#else
    srand(time(NULL));
#endif
    signal(SIGUSR1, &start);
    log(CLIENT, "Current pid: %d", getpid());
#ifdef DAEMONIZE
	daemonize(argv[0]);
#endif
    return start_client(&process_srv_message, &main_dispatcher, &q);
}

#elif defined SRV
#   include "../include/srv.h"

int main(int argc, char **argv) {
#ifdef DAEMONIZE
	daemonize(argv[0]);
#endif
    return start_server(&process_client_message);
}

#else

int main(int argc, char **argv) {
    err(-1, "Compile define option is required");
    return 1;
}

#endif
