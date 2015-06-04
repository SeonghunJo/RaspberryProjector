#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define TRUE 1
#define KB 1024

int main(int argc, char **argv)
{
	int sockfd;
	int status;
	int optvalue;
	int optlen;

	int snd_size, rcv_size;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);

	optvalue = TRUE;
	optlen = sizeof(optvalue);
	
	status = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optvalue, optlen);

	if(status)
		puts("error :: setsocket() SO_BROADCAST ");

	// retrive socket option
	status = getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optvalue, &optlen);

	if(status)
		puts("error :: getsocket() SO_BROADCAST ");

	printf("SO_BROADCAST : %s\n", optvalue == 1 ? "SETTING" : "NON-SETTING");
	
	getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen);
	printf("Send Buffer Size : %d KB, %d\n" , snd_size/KB, snd_size);
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen);
	printf("Recv Buffer Size : %d KB, %d\n" , rcv_size/KB, rcv_size);
	//setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen);

	//setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen);
	return 0;
}
