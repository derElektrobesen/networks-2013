#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


#define LOCK_FILE_NAME "/var/run/daemon.pid"
#define LOCK_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
int lockfile(int);
void daemonize(const char *);
int is_running(const char *);

int main(int argc, char *argv[])
{
    int i = 0;
    daemonize(argv[0]);    
    for (i = 0; i < 3; i++) {
        syslog(LOG_INFO, "i am a daemon");
        sleep(30);
    }

    return 0;
}

void daemonize(const char *cmd)
{
    int i;
    pid_t pid;
    struct rlimit rl;
    int block_fd;
    //struct sigaction sa;

    if ((block_fd = is_running(NULL)) == -1) {
        syslog(LOG_ERR, "daemon is already running");
        exit(1);
    }

    umask(0);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0){
        perror("geterrlimit failure");
        //log(OTHER, "getrlimit failure!");
        return;
    }
    
    if ((pid = fork()) < 0) {
        return;
    } else if (pid != 0){
        exit(0);
    }
    setsid();

    /*sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < ) {
        log(OTHER, "sigaction failure");
        return;
    }
    if ((pid = fork()) < 0) {
        log(OTHER, "fork failure");
        return;
    }
    else if (pid != 0)
        exit(0);
    */

    if (chdir("/") < 0) {
        //log(OTHER, "chdir failure");
        return;
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max && i != block_fd; i++)
        close(i);
    
    /*close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    */
    
    /*fd0 = fdopen("/dev/null", O_RDDR);
    fd1 = dup(0);
    fd2 = dup(0);*/

    
    openlog(cmd, LOG_CONS | LOG_PID, LOG_DAEMON);
    /*if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "incorrect file descriptors %d, %d, %d", fd0, fd1, fd2);
        //log(OTHER, "incorrect file descriptors");
        exit(1);
        
    }*/
}

int is_running(const char *file_name)
{
    int fd;
    char buf[16];
    fd = open(LOCK_FILE_NAME, O_RDWR | O_CREAT, LOCK_MODE);
    if (fd < 0) {
        syslog(LOG_ERR, "could not open file %s", LOCK_FILE_NAME);
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return -1;
        }
        syslog(LOG_ERR, "could not open the file %s", LOCK_FILE_NAME);
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%1d", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return fd;
}


int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}
