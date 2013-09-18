#include "network.h"
#include "config.h"

#include <arpa/inet.h>
#include <strings.h>

#define message "Hello, Pavel!\n"
#define MAXSLEEP 1000


int main(int argc, char *argv[])
{

    int sock;
    struct sockaddr_in addr;    
    in_addr_t ip_addres;
    inet_ip_addr addres;
    struct hostent *server;
    
    server = gethostbyname(argv[1]);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket syscall error!\n");
        exit(-1);
    }

    int flags;
    fd_set set;

    fcntl(sock, F_GETFL, &flags);
    flags |= O_NONBLOCK;
    fcntl(sock, F_SETFL, flags);
    FD_ZERO(&set);
    FD_SET(sock, &set);

       
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);
    addr.sin_port = htons(PORT);
    printf("IP = %s", server->h_name);

    if(connect_retry(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
       	perror("connection failure!");
        exit(1);
    }
    printf("Client has connected!\n");

    if (send(sock, message, sizeof(message), 0) > 0)
	printf("Client has sent message: %s", message);
 
    return 0;
}


int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
	int nsec;
	for (nsec = 1; nsec <= MAXSLEEP; nsec <<= 1) {
        printf("connecting ...\n");        
		if (connect(sockfd, addr, alen) == 0)
        
			return 0;
		if (nsec <= MAXSLEEP/2)
			sleep(1);//sleep(nsec);
	}
	return -1;
}
