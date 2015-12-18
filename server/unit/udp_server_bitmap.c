#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAXBUF 8192
#define BLOCK_SIZE 8192

char *buf = NULL;

int main()
{
	int ssock;
	int clen;
	struct sockaddr_in client_addr, server_addr;

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
	

	int totalLength = 0;
	FILE *out = fopen("out.bmp", "w");
	char block[BLOCK_SIZE];

	buf = (char *)malloc(1920 * 1080 * 3);
	if(buf == NULL)
	{
		printf("Buffer Created Fail\n");
		return 1;
	}

	int cnt = 0;

	while(1) {
		int length = recvfrom(ssock, (void *)block, BLOCK_SIZE, 0, (struct sockaddr*)&client_addr, &clen);

		memcpy(buf + totalLength, block, length);
		totalLength += length;
		printf("current %d, total %d (%d)\n", length, totalLength, ++cnt);
		
		if(totalLength >= 1920 * 1000 * 3)
		{
			printf("Image receive complete!\n");
			break;
		}
	}

	fwrite(buf, totalLength, 1, out);
	fclose(out);
	close(ssock);	

	return 0;
}
