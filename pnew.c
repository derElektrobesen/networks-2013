#include "network.h"
#include "config.h"

#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>


#define message "Hello, Pavel!\n"
#define MAXSLEEP 1000


int main(int argc, char *argv[])    // add check of parameters
{
    int sock;
    int err;
    struct sockaddr_in *addr; 
    struct addrinfo hint;
    struct addrinfo *ailist, *aip;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(argv[1], PORT_CHAR, &hint, &ailist)) < 0) {
        err = errno;        
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));        
        exit(-1);
    }
    
    if ((sock = socket(ailist->ai_family, ailist->ai_socktype, ailist->ai_protocol)) < 0) {
        perror("[Client] socket failure\n");
        exit(-1);        
    }
    
    if (connect_retry(sock, ailist->ai_addr, ailist->ai_addrlen) < 0) {
        perror("[Client] connection failure\n");
        exit(-1);
    } else if (send(sock, message, sizeof(message), 0) > 0)
        printf("Client has sent message: %s", message);

        
    
    /*for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if (connect(sock, ailist->ai_addr, ailist->ai_addrlen)  < 0)        
        {
       	    printf("connection failure!\n");
        } else {
            if (send(sock, message, sizeof(message), 0) > 0)
	        printf("Client has sent message: %s", message);
        }
        printf("Trying another connection!\n");
    }*/
    freeaddrinfo(ailist);
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
