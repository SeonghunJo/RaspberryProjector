#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAXBUF 25


int decode_frame(const char *buffer, int length, void *samples)
{
    int n_samples;
    struct jpeg_error_mgr err;
    struct jpeg_decompress_struct cinfo = {0};

    /* create decompressor */
    jpeg_create_decompress(&cinfo);
    cinfo.err = jpeg_std_error(&err);
    cinfo.do_fancy_upsampling = FALSE;

    /* set source buffer */
    jpeg_mem_src(&cinfo, (unsigned char *)buffer, length);

    /* read jpeg header */
    jpeg_read_header(&cinfo, 1);

    /* decompress */
    jpeg_start_decompress(&cinfo);

    /* read scanlines */
    while (cinfo.output_scanline < cinfo.output_height)
    {
        n_samples = jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &samples, 1);
        samples += n_samples * cinfo.image_width * cinfo.num_components;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}

int main()
{
	int ssock;
	int clen, num=0;
	struct sockaddr_in server_addr;
	char buf[MAXBUF];

	if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
		exit(1);
	}

	clen = sizeof(server_addr);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(3317);

	printf("Try Connect\n");
	if(connect(ssock, (struct sockaddr *)&server_addr, clen) < 0) {
		perror("connect error : ");
		exit(1);
	}

	printf("Run Loop\n");
	while(1)
	{
		sprintf(buf, "test (%d)", num++);

		if(write(ssock, buf, MAXBUF) <= 0) {
			perror("write error : ");
			exit(1);
		}

		if(read(ssock, buf, MAXBUF) <= 0) {
			perror("read error : ");
			exit(1);
		}

		printf("\nread : %s\n\n", buf);

		sleep(1);
	}

	close(ssock);

	return 0;
}

