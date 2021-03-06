#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAXBUF 16384

int main()
{
	int ssock;
	int clen;
	struct sockaddr_in client_addr, server_addr;
	char buf[MAXBUF];

	if ((ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket error : ");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(3318);

	if(bind(ssock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind error : ");
		exit(1);
	}

	clen = sizeof(client_addr);

	int num = 0;
	while(1) {
		int length = recvfrom(ssock, (void *)buf, MAXBUF, 0, (struct sockaddr*)&client_addr, &clen);
		printf("%d\n", length);
	
		if(length > 0)
			num++;
		if(num > 30)
		{
			printf("30 Frame\n\n");
			num = 0;
		}

		sprintf(buf, "%d packet is received at server", length);
		length = strlen(buf);
		sendto(ssock, (void *)buf, length, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
	}
	close(ssock);

	return 0;
}
