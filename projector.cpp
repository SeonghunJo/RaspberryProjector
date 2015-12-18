// Common Header
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
// Framebuffer
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
// Socket
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
// Jpeg Converter
#include "jpeglib.h"

#define MAX_BUF 16384

using namespace std;

typedef struct
{
	unsigned int size;	// header size
	int width, height;	// image width, height
	unsigned short int planes;
	unsigned short int bpp;	// bit per pixels 1,4,8,16,24,32
	unsigned int compression;
	unsigned int imagesize;
	int xresolution, yresolution;
	unsigned int colours;
	unsigned int impcolours;
} INFOHEADER;

// 'global' variables to store screen info
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

struct fb_var_screeninfo orig_vinfo;

int fbfd = 0;       // framebuffer file descriptor
char *fbp = 0;      // FRAME Buffer Mapped Memory

long int screensize = 0;

// Loaded Image Buffer
unsigned char *buffer = NULL;

// SOCKET
int server_socket;
int client_socket;
unsigned int clen, data_len;

struct sockaddr_in client_addr, server_addr;
int fd;
pid_t pid;

fd_set read_fds, tmp_fds;
char buf[MAX_BUF] = "Test!";

// CLASSES
class Client
{
private:
    int resX, resY; // 클라이언트의 화면 크기
    int remainCount;

    Client()
    {

    }
};

// SOURCES

// RGB24 is 3 consecutive bytes (RGB)
void put_pixel_RGB24(int x, int y, int r, int g, int b)
{
	// calculate the pixel's byte offset inside the buffer
	// note: x * 3 as every pixel is 3 consecutive bytes
	unsigned int pix_offset = x * 3 + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	*((char *)(fbp + pix_offset)) = r;
	*((char *)(fbp + pix_offset + 1)) = g;
	*((char *)(fbp + pix_offset + 2)) = b;
}

void draw() {

	int bufferPos = 54;
	unsigned char r, g, b;

	int ir, ig, ib;

	int x, y;
	//for (y = 0; y < vinfo.yres; y++) {
	for(y=vinfo.yres-1; y>=0; y--) { // Image Reverse
		for (x=0; x < vinfo.xres; x++) {
			int c = 16 * x / vinfo.xres;

            ir = buffer[bufferPos];
            ig = buffer[bufferPos + 1];
            ib = buffer[bufferPos + 2];

            put_pixel_RGB24(x, y, ir, ig, ib);
            printf("%d %d %d\n", r, g, b);
            bufferPos += 3;
		}
	}
}

// ===========================BITMAP==========================


// ********** Verify if the file is BMP *********
void isBMP(FILE* arq, INFOHEADER info){
        char type[3];
        unsigned short int bpp;
        fseek(arq,0,0);
        fread(type,1,2,arq);
        type[2] = '\0';

        fseek(arq,28,0);
        fread(&bpp,1,2,arq);

		printf("Bitmap Type : %s\n", type);
		printf("Bitmap BPP : %d\n", bpp);

        if (strcmp(type,"BM") || (bpp != 24)){
                printf("\nThe file is not BMP format or is not 24 bits\n");
                exit(0);
        }
}

// ********** Read BMP info from file **********
INFOHEADER readInfo(FILE* arq){
        INFOHEADER info;

        // Image Width in pixels
        fseek(arq,18,0);
        fread(&info.width,1,4,arq);

        // Image Height in pixels
        fseek(arq,22,0);
        fread(&info.height,1,4,arq);

        // Color depth, BPP (bits per pixel)
        fseek(arq,28,0);
        fread(&info.bpp,1,2,arq);

        // Compression type
        // 0 = Normmally
        // 1 = 8 bits per pixel
        // 2 = 4 bits per pixel
        fseek(arq,30,0);
        fread(&info.compression,1,4,arq);

        // Image size in bytes
        fseek(arq,34,0);
        fread(&info.imagesize,1,4,arq);

        // Number of color used (NCL)
        // value = 0 for full color set
        fseek(arq,46,0);
        fread(&info.colours,1,4,arq);

        // Number of important color (NIC)
        // value = 0 means all collors important
        fseek(arq,50,0);
        fread(&info.impcolours,1,4,arq);

        return(info);
}

class Drawer
{
    // 프레임버퍼에 지속적으로 메모리를 제공하는 클래스
    // 싱글턴으로 작성해야함

    Drawer()
    {

    }


};

// application entry point
int main(int argc, char* argv[])
{
	// Open the framebuffer device file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		printf("Error: cannot open framebuffer device.\n");
		return(1);
	}

	printf("The framebuffer device opened.\n");

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable screen info.\n");
	}

	// Store for rest (copy var_info to vinfo_orig)
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

	//vinfo.bits_per_pixel = 24; // 3바이트로 고정

	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
		printf("Error setting variable information. \n");
	}

	// GET fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed screen info.\n");
	}

	printf("Display info %dx%d, %d bpp\n",
			vinfo.xres, vinfo.yres,
			vinfo.bits_per_pixel );

	// map framebuffer to user memory
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8; //finfo.smem_len;

    FILE *in = fopen("3.bmp", "rb");

	// Get Bitmap Info
	INFOHEADER info;
	info = readInfo(in);

	int height, width, bpp;
	height = info.height;
	width = info.width;
	bpp = info.bpp;

	printf("File width %d, height %d, bpp %d\n", width, height, bpp);

	//buffer = (char *)malloc(1920 * 1080 * 3);
	buffer = (unsigned char *)malloc(height * width * bpp/8);
	if(buffer == NULL) {
		printf("Create Buffer Failed");
	}

	int nread = fread(buffer, sizeof(unsigned char), width * height * bpp/8, in);
	printf("%d Memory Read\n", nread);
	sleep(1);

	// Draw

	fbp = (char*)mmap(0,
					  screensize,
					  PROT_READ | PROT_WRITE,
					  MAP_SHARED,
					  fbfd, 0);

	if ((int) fbp == -1) {
		printf("Failed to mmap.\n");
	}
	else {
		// draw..
		//draw();
		//memcpy(fbp, buffer, (width * height * bpp/8)/2);

		sleep(1);
	}

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

    printf("Server is running.\n");
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
						printf("Write data to client at %d\n", fd);
						write(1, buf, MAX_BUF);
						write(fd, buf, MAX_BUF); // ECHO
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


    printf("fbp memory munmap\n");
	// cleanup
	munmap(fbp, screensize);
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		printf("Error re-setting variable information.\n");
	}

	close(fbfd);

	return 0;
}


