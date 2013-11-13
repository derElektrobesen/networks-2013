#include "network.h"
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef CLI
#include <time.h>

#   include "cli.h"

/* Не знаю как перенести в Makefile */
#define CLI_LOCK_FILE_NAME "/var/run/cli.pid"
#define SRV_LOCK_FILE_NAME "/var/run/srv.pid"
#define LOCK_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}

int is_running(const char *file_name)
{
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

void daemonize(const char *cmd, const char *file_name)
{
    int i;
    pid_t pid;
    struct rlimit rl;
    int block_fd;

    if ((block_fd = is_running(file_name)) == -1) {
        syslog(LOG_ERR, "%s is already running", cmd);
        exit(1);
    }

    umask(0);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0){
        /* add log */
        return;
    }
    
    if ((pid = fork()) < 0) {
        return;
    } else if (pid != 0){
        exit(0);
    }
    setsid();

    if (chdir("/") < 0) {
        /*add log*/
        return;
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max && i != block_fd; i++)
        close(i);
   
    openlog(cmd, LOG_CONS | LOG_PID, LOG_DAEMON);
}


struct sockets_queue q;

void start(int sig) {
    //recieve_file("file", (const unsigned char *)"\x3d\xfe\xc4\x37\xab\x06\x1d\x83\x8b\xbb\xc4\xca\x1f\xd2\xdf\xcf", 10, &q);
    //recieve_file("RobertLove.pdf", (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e", 3244909, &q);
    recieve_file("RobertLove.pdf", (const unsigned char *)"\x19\x01\x67\x36\x6f\x14\xce\x7e\x9b\x26\x8d\x72\xad\x87\xf1\x48", 3244909, &q);
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
	/*daemonize(argv[0], SRV_LOCK_FILE_NAME);*/
    return start_server(&process_client_message);
}

#else

int main(int argc, char **argv) {
	/*daemonize(argv[0], CLI_LOCK_FILE_NAME);*/
    err(-1, "Compile define option is required");
    return 1;
}

#endif
