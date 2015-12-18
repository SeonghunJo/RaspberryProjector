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
	int clen, num=0;
	struct sockaddr_in server_addr;

	FILE *in = fopen("window.jpg", "rb");
	//FILE *in = fopen("simba.jpg", "rb");
	
	if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
		exit(1);
	}

	clen = sizeof(server_addr);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("112.108.40.66");
	//server_addr.sin_addr.s_addr = inet_addr("121.129.10.166");
	server_addr.sin_port = htons(3317);

	printf("Try Connect\n");
	if(connect(ssock, (struct sockaddr *)&server_addr, clen) < 0) {
		perror("connect error : ");
		exit(1);
	}
	
	int nwrite = 0;
	int len = 0;
	int type = 0;
	
	char name[10]="seonghun";
	char *buf = NULL;
	type = 2;
	len = 8;
	buf = (char *)calloc(sizeof(char), len + 8);

	memcpy(buf, (void*)&type, sizeof(int));
	memcpy(buf+4, (void*)&len, sizeof(int));
	memcpy(buf+8, name, 8);
	nwrite = write(ssock, buf, 16);
	
	printf("nwrite %d\n", nwrite);
	return 0;

	type = 5;
	fseek(in, 0, SEEK_END);
	len = ftell(in);
	fseek(in, 0, SEEK_SET);
	
	buf = (char *)calloc(sizeof(char), len + 8);
	char *imageBuf = (char *)calloc(sizeof(char), len);
	memcpy(buf, (void *)&type, sizeof(int));
	memcpy(buf+4, (void *)&len, sizeof(int));
	fread(imageBuf, sizeof(char), len, in);
	memcpy(buf+8, imageBuf, len);
	
	printf("type %d, len %d\n", type, len);
	for(int i=0; ;) {
		int totalBytes = len + 8;
		int remainBytes = len + 8;
		while(remainBytes > 0)
		{
			int nwrite = write(ssock, buf + totalBytes - remainBytes, remainBytes);
			if(nwrite < 0) 
			{
				perror("write error");
			}
			remainBytes -= nwrite;
		}
	}

	close(ssock);

	return 0;
}

