#include "network.h"

int create_socket() {
    int sock = -1, listener = -1;
	struct sockaddr_in addr;
	char buf[1024];
	int bytes_read;
	fd_set set;
	struct timeval tv;
     
	tv.tv_sec = 10;
	tv.tv_usec = 0; 

	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failure!");
		exit(1);
    }
	printf("\nlistener = %d\n", listener);

    addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
	if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind failure!");
		exit(1);
	}

	if (listen(listener, SELECT_QUEUE_LEN) < 0) {
		perror("listen failure!");
		exit(1);
	}

	int err;
	int flag;   

	while(1) {
		if ((err = select(listener + 1, &set, NULL, NULL, &tv)) > 0) {
			if (FD_ISSET(listener, &set)) {
				sock = accept(listener, NULL, NULL);
				printf("\nsock = %d\n", sock);
				if(sock < 0) {
					perror("accept failure!");
					exit(1);
				}
	
				flag = 1; 
				while(flag) { 
					bytes_read = recv(sock, buf, 1024, 0);
		  	
					if(bytes_read <= 0) 
						flag = 0;
					else {
						printf("Server received message: %s\n", buf);
					}
				}  
	 			close(sock);
			}
		
		} else if(err == 0) {
	   		 printf("Time is up!\n");
	   		 exit(1);
		}
	}
	return 0;
}
