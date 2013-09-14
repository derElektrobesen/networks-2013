#include "network.h"
#include "config.h"
#include "arpa/inet.h"

#define message "Hello, Pavel!\n"
#define MAXSLEEP 10


int main()
{

    int sock;
    struct sockaddr_in addr;    
    in_addr_t ip_addres;
    inet_ip_addr addres;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket syscall error!\n");
        exit(-1);
    }
       
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); 
    ip_addres = inet_pton(AF_INET, HOST, &addres);;//htonl(INADDR_LOOPBACK);
    addr.sin_addr.s_addr = ip_addres;

    if(connect_retry(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
       	perror("connection failure!");
        exit(1);
    }

    if (send(sock, message, sizeof(message), 0) > 0)
	printf("Client has sent message: %s", message);
 
    return 0;
}


int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
	int nsec;
	for (nsec = 1; nsec <= MAXSLEEP; nsec <<= 1) {
		if (connect(sockfd, addr, alen) == 0)
			return 0;
		if (nsec <= MAXSLEEP/2)
			sleep(nsec);
	}
	return -1;
}
