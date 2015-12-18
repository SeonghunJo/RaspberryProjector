// HEADER

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAX_BUF 16384

int main()
{
	int server_socket;
	int client_socket;
	int clen, data_len;
	
	int fd;
	pid_t pid;

	struct sockaddr_in client_addr, server_addr;
	fd_set read_fds, tmp_fds;
	char buf[MAX_BUF] = "";

	/* Create Socket */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
		exit(1);
	}

	/* Make Server Option */
	clen = sizeof(client_addr);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(3317);

	/* Set Server Options */
	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind error : ");
		exit(1);
	}

	/* listening */
	if (listen(server_socket, 5) < 0) {
		perror("listen error : ");
		exit(1);
	}

	/* Set File Descriptor */
	FD_ZERO(&read_fds);
	FD_SET(server_socket, &read_fds);
	
	while(1)
	{
		tmp_fds = read_fds;

		if (select(FD_SETSIZE, &tmp_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 1) {
			perror("select error : ");
			exit(1);
		}
		
		for(fd=0; fd<FD_SETSIZE; fd++)
		{
			if(FD_ISSET(fd, &tmp_fds)) {
				
				if(fd == server_socket)
				{
					client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &clen);

					FD_SET(client_socket, &read_fds);
					printf("new client %d descriptor accepted\n", client_socket);
				}
				else
				{
					data_len = read(fd, buf, MAX_BUF);
					if(data_len > 0)
					{
						printf("Write data to client at %d, size %d\n", fd, data_len);
						if(data_len >= 4)
						{
							int val = 0;
							val += (buf[3] & 0xFF);
							val += (buf[2] & 0xFF) << 8;
							val += (buf[1] & 0xFF) << 16;
							val += (buf[0] & 0xFF) << 24;

							printf("HEADER LENGTH : %d\n", val);
						}
						if(data_len >= 8)
						{
							int val = 0;
							val += (buf[7] & 0xFF);
							val += (buf[6] & 0xFF) << 8;
							val += (buf[5] & 0xFF) << 16;
							val += (buf[4] & 0xFF) << 24;

							printf("HEADER TYPE : %d\n", val);
						}
						printf("-------------------------------");
						write(1, buf, data_len);
						printf("\n");

						write(fd, buf, data_len); // ECHO
					}
					else if(data_len == 0)
					{
						close(fd);
						FD_CLR(fd, &read_fds);

						printf("close client at %d", fd);
					}
					else if(data_len < 0) // error
					{
						perror("read error : ");
						exit(1);
					}
				}
			}
		}
	}

	return 0;
}
