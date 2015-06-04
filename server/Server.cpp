// Common Header
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h> 
// IP Address
#include <ifaddrs.h>
#include <netdb.h>

// Framebuffer
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
// Socket
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
// Jpeg Converter
#include <jpeglib.h>
#include <turbojpeg.h>

#include "Bitmap.h"
#include "JPEGConverter.h"

#include <iostream>

//#define MAX_BUF 16384
#define MAX_BUF 87380
#define MAX_CLIENT_NUM 30

#define HEADER_SIZE 8

#undef DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

using namespace std;

// 'global' variables to store screen info
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

struct fb_var_screeninfo orig_vinfo;

int fbfd = 0;       // framebuffer file descriptor
char *fbp = 0;      // FRAME Buffer Mapped Memory

char *waitScreen = 0;
int waitScreenSize = 0;

long int screensize = 0;

// Loaded Image Buffer
unsigned char *buffer = NULL;

// SOCKET
int server_socket;
int client_socket;
unsigned int clen;
int data_len;
struct sockaddr_in client_addr, server_addr;

fd_set read_fds, tmp_fds;

#define RECV_NAME			2
#define SEND_USER_LIST		3

#define CHANGE_FOCUS_STATUS	4 // RECV or SEND
#define RECV_IMAGE			5

int focus_fd; // Current Focus fd
int ready_fd; // new Focus fd

// 현재 포커스 fd가 0보다 크면 존재
// 

int userCount; // User Count
// CLASSES
class Client
{
public:
	int fd;
	int resX, resY; // 클라이언트의 화면 크기
	int bpp;

	int header_length;
	int header_type;
	int remainBytes;

	int userID;
	char *userName;
	unsigned int userNameLength;

	char *recvBuffer;
	unsigned int recvOffset;

	// For BITMAP Images
	unsigned char *rawBuffer;
	unsigned int rawBufferSize;

	int jpegWidth, jpegHeight, jpegSubsamp;
	unsigned char *jpegBuffer;
	unsigned int jpegOffset;

	tjhandle jpegDecompressor;
	
	Client()
	{
		init();
	}

	Client(int socket, int id)
	{
		fd = socket;
		userID = id;
		resX = vinfo.xres;
		resY = vinfo.yres;
		bpp = vinfo.bits_per_pixel;

		init();
	}


	int getMyAddress()
	{
		struct ifaddrs *ifaddr, *ifa;
		int family, s;
		char host[NI_MAXHOST];

		if (getifaddrs(&ifaddr) == -1)
		{
			DEBUG_PRINT("getifaddrs");
			exit(EXIT_FAILURE);
		}

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr == NULL)
				continue;

			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			// if wlan0
			if ((strcmp(ifa->ifa_name, "eth0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
			{
				if (s != 0)
				{
					DEBUG_PRINT("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(EXIT_FAILURE);
				}
				printf("\tInterface : <%s>\n", ifa->ifa_name);
				printf("\t  Address : <%s>\n", host);
			}

			if ((strcmp(ifa->ifa_name, "wlan0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
			{
				if (s != 0)
				{
					DEBUG_PRINT("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(EXIT_FAILURE);
				}
				printf("\tInterface : <%s>\n", ifa->ifa_name);
				printf("\t  Address : <%s>\n", host);
			}
		}

		freeifaddrs(ifaddr);
	}

	~Client()
	{
		if (jpegBuffer != NULL)
		{
			free(jpegBuffer);
			jpegBuffer = NULL;
		}
		DEBUG_PRINT("User Count--\n");
		userCount--;

		if (fd == focus_fd) // 현재 클라이언트가 포커스 상태인데 종료했을 경우
		{
			DEBUG_PRINT("----------------------HOST IS OUT----------------------\n");
			if (userCount == 0) // 만약 자신이 마지막으로 나가는 것이면 focus_fd을 -1으로 바꾼다.
			{
				focus_fd = -1;
				ready_fd = -1;
				system("clear");
				getMyAddress();
			}
			else
			{
				DEBUG_PRINT("----------------------HOST CHANGE CALL----------------------\n");
				ready_fd = 0;
			}
		}
		tjDestroy(jpegDecompressor);
	}

	int init()
	{
		DEBUG_PRINT("New client init : %d\n", fd);

		jpegDecompressor = tjInitDecompress();

		
		recvOffset = 0;
		recvBuffer = NULL;
	
		userName = NULL;
		userNameLength = 0;

		jpegOffset = 0;
		jpegBuffer = NULL;

		remainBytes = -1;

		DEBUG_PRINT("Set RawBuffer %d %d %d\n", resX, resY, bpp);

		rawBufferSize = resX * resY * bpp / 8;
		rawBuffer = new unsigned char[rawBufferSize + 1];

		if (userCount == 0)
		{
			DEBUG_PRINT("[SET NEW FOCUS USER : %d]\n", fd);
			focus_fd = -1;
			ready_fd = fd;
		}

		userCount++;
	}

	int decode_frame(unsigned char *samples)
	{
		tjDecompressHeader2(jpegDecompressor, jpegBuffer, jpegOffset, &jpegWidth, &jpegHeight, &jpegSubsamp);
		tjDecompress2(jpegDecompressor, jpegBuffer, jpegOffset, samples, jpegWidth, 0/*pitch*/, jpegHeight, TJPF_BGR, TJFLAG_FASTDCT);

		return jpegWidth * jpegHeight * 3;
	}

	void draw() // 프레임 버퍼에 그리는 함수
	{
		if (jpegOffset > 0) // JPEG 버퍼가 있으면
		{
			//int ret = decode_frame(rawBuffer);
			tjDecompressHeader2(jpegDecompressor, jpegBuffer, jpegOffset, &jpegWidth, &jpegHeight, &jpegSubsamp);

			if (jpegWidth == resX && jpegHeight == resY)
			{
				DEBUG_PRINT("FBP DIRECT!\n");
				tjDecompress2(jpegDecompressor, jpegBuffer, jpegOffset, (unsigned char *)fbp, jpegWidth, 0/*pitch*/, jpegHeight, TJPF_BGR, TJFLAG_FASTDCT);
				//memcpy(fbp, rawBuffer, rawBufferSize);
			}
			else
			{
				//
				//tjDecompress2(jpegDecompressor, jpegBuffer, jpegOffset, rawBuffer, 1024, 0/*pitch*/, 768, TJPF_BGR, TJFLAG_FASTDCT);
				//int bufferPos = 0;
				//unsigned char r, g, b;
				//int x, y;

				//DEBUG_PRINT("yres %d xres %d\n", resY, resX);
				//for (y = 0; y < resY; y++) { // Image Reverse
				//	for (x = 0; x < resX; x++) {

				//		r = rawBuffer[bufferPos]; // *(buffer + bufferPos)
				//		g = rawBuffer[bufferPos + 1]; // *(buffer + bufferPos + 1)
				//		b = rawBuffer[bufferPos + 2]; // *(buffer + bufferPos + 2)

				//		if (x < vinfo.xres && y < vinfo.yres) //화면 크기 이상은 그리지 않는다.
				//		{
				//			put_pixel_RGB24(x, y, r, g, b);
				//		}

				//		bufferPos += 3;
				//	}
				//}
				//
			}
		}
	}

	/*
	void drawPixels() // 
	{
		if (jpegOffset > 0) // JPEG 버퍼가 있으면
		{
			int w, h;

			int ret = jpegConverter.decode_frame((const char *)jpegBuffer, jpegOffset, rawBuffer, &w, &h);
			DEBUG_PRINT("Decode Frame return %d, %x\n", ret, rawBuffer);

			int bufferPos = 0;
			unsigned char r, g, b;
			int x, y;

			for (y = 0; y < h; y++) { // Image Reverse
				for (x = 0; x < w; x++) {

					r = rawBuffer[bufferPos]; // *(buffer + bufferPos)
					g = rawBuffer[bufferPos + 1]; // *(buffer + bufferPos + 1)
					b = rawBuffer[bufferPos + 2]; // *(buffer + bufferPos + 2)

					if (x < vinfo.xres && y < vinfo.yres) //화면 크기 이상은 그리지 않는다.
					{
						put_pixel_RGB24(x, y, r, g, b);
					}

					bufferPos += 3;
				}
			}
		}
		DEBUG_PRINT("Draw Pixel End\n");
	}
	*/

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

	static void counter()
	{
		static int count;
		static int totalCount;
		count++;
		if (count > 30)
		{
			DEBUG_PRINT("--------------------- 30 Frame [%d] ---------------------\n", totalCount++);
			count = 0;
		}
	}

	// 청크 (현재 패킷)
	// 데이터 (총 데이터)
	int receive(char *buf, int len)
	{
		int chunkSize = len;
		int chunkRead = 0;

		while (chunkSize - chunkRead > 0) // 남은 청크 데이터가 있을 경우
		{
			// 만약 현재 더받아야할 데이터가 없으면 패킷 헤더를 확인한다.
			if (remainBytes <= 0) // 헤더 처리부
			{
				memcpy((void *)&header_type, buf + chunkRead, sizeof(int));
				chunkRead += sizeof(int);
				memcpy((void *)&header_length, buf + chunkRead, sizeof(int));
				chunkRead += sizeof(int);
				DEBUG_PRINT("[TYPE : %d, LENGTH : %d, ID : %d]\n", header_type, header_length, userID);

				if (header_type > 10 || header_type < 0)
				{
					DEBUG_PRINT("[INVALID HEADER TYPE!]\n");

					return -1;
				}
				// Common
				remainBytes = header_length;
				recvOffset = 0;

				if (header_length > 0)
				{
					if (recvBuffer != NULL)
					{
						delete recvBuffer;
						recvBuffer = NULL;
					}
					recvBuffer = new char[header_length + 1]; // IF HEADER LENGTH is 5, Array 0~4 + terminate(\0)
					recvBuffer[header_length] = '\0';
				}

				if (header_type == RECV_NAME)
				{
					if (userName == NULL)
					{
						userName = new char[header_length + 1];
						userName[header_length] = '\0';
					}
				}
				else if (header_type == RECV_IMAGE)
				{

				}
				else if (header_type == CHANGE_FOCUS_STATUS)
				{
				}
			}
			else  // 데이터 처리부
			{
				// 읽어야할 바이트가 현재 남은 청크의 크기보다 클경우
				if (remainBytes > (chunkSize - chunkRead))
				{
					// 다음 청크를 기다려야한다. 따라서 현재 위치부터 청크의 끝까지 읽는다.
					int readSize = chunkSize - chunkRead;
					if (header_type == RECV_IMAGE)
					{
						memcpy(recvBuffer + recvOffset, buf + chunkRead, readSize);
						recvOffset += readSize;

					}
					if (header_type == RECV_NAME)
					{
						memcpy(recvBuffer + recvOffset, buf + chunkRead, readSize);
						recvOffset += readSize;
					}

					chunkRead += readSize;
					remainBytes -= readSize;
				}
				// 읽어야할 바이트가 현재 남은 청크의 크기보다 작을거나 같을 경우
				else
				{
					// 현재 청크 위치에서 읽어야할 바이트만큼 읽는다.
					DEBUG_PRINT("[Done!] %s\n", userName);

					int readSize = remainBytes;
					memcpy(recvBuffer + recvOffset, buf + chunkRead, readSize);
					recvOffset += readSize;

					if (header_type == RECV_IMAGE)
					{
						if (jpegBuffer != NULL)
						{
							delete jpegBuffer;
							jpegBuffer = NULL;
						}
						jpegBuffer = new unsigned char[recvOffset + 1];
						jpegBuffer[recvOffset] = '\0';
					
						memcpy(jpegBuffer, recvBuffer, recvOffset);
						jpegOffset = recvOffset;

						if (focus_fd == fd) // 포커스가 있는 사용자만 화면에 그린다.
						{
							//DEBUG_PRINT("DRAW : %s\n", userName);
							draw();
							//counter();
						}
						else
						{
							//DEBUG_PRINT("PASS : %s\n", userName);
						}
					}
					if (header_type == RECV_NAME)
					{
						memcpy(userName, recvBuffer, recvOffset);
						userNameLength = recvOffset;
						userName[recvOffset] = '\0';
						DEBUG_PRINT("[Set User Name (%d) : %s]\n", fd, userName);
					}
					if (header_type == CHANGE_FOCUS_STATUS)
					{
						int changeFocusID;
						memcpy(&changeFocusID, recvBuffer, sizeof(int));
						ready_fd = changeFocusID;
						DEBUG_PRINT("FOCUS CHANGE REQUEST %d -> %d]\n", focus_fd, ready_fd);
					}

					DEBUG_PRINT("[END DONE! %s]\n", userName);

					chunkRead += readSize;
					remainBytes -= readSize;
				}

			}

		} // End of while	

		return 0; // Success
	}

};



int setFrameBuffer()
{
	// Open the framebuffer device file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		DEBUG_PRINT("Error: cannot open framebuffer device.\n");
		return(1);
	}

	DEBUG_PRINT("The framebuffer device opened.\n");

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		DEBUG_PRINT("Error reading variable screen info.\n");
	}

	// Store for rest (copy var_info to vinfo_orig)
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

	vinfo.bits_per_pixel = 24; // 3바이트로 고정
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
		DEBUG_PRINT("Error setting variable information. \n");
	}

	// GET fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		DEBUG_PRINT("Error reading fixed screen info.\n");
	}

	DEBUG_PRINT("Display info %dx%d, %d bpp\n",
		vinfo.xres, vinfo.yres,
		vinfo.bits_per_pixel);

	DEBUG_PRINT("Display finfo %d\n", finfo.line_length);

	// map framebuffer to user memory
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8; //finfo.smem_len;

	fbp = (char*)mmap(0,
		screensize,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fbfd, 0);

	DEBUG_PRINT("Memory Mapping\n");

	if ((int)fbp == -1) {
		DEBUG_PRINT("Failed to mmap.\n");
		return 1;
	}

	return 0;
}

void unsetFrameBuffer()
{
	DEBUG_PRINT("fbp memory munmap\n");
	// cleanup
	munmap(fbp, screensize);
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		DEBUG_PRINT("Error re-setting variable information.\n");
	}

	close(fbfd);
}

int getSocketOption(int socket)
{
	int snd_size = 0;
	int rcv_size = 0;
	socklen_t optlen;

	if (getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen) == 0)
	{
		DEBUG_PRINT("Send Buffer Size : %d KB (%d) \n", snd_size / 1024, snd_size);
	}

	if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen) == 0)
	{
		DEBUG_PRINT("Receive Buffer Size : %d KB (%d) \n", rcv_size / 1024, rcv_size);
	}
}

int setSocketOption(int socket)
{
	int snd_size = MAX_BUF;
	int rcv_size = MAX_BUF;

	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size));

	setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &rcv_size, sizeof(rcv_size));

	DEBUG_PRINT("---------SET SOCKET OPTION %d---------\n", socket);
	getSocketOption(socket);
}

Client *client[MAX_CLIENT_NUM];

void* userListThread(void *data)
{
	// 현재 포커싱된 사용자에게 지속적으로 현재 사용자 리스트를 넘긴다.
	while (1)
	{
		if (focus_fd > 0 && userCount > 1)
		{
			DEBUG_PRINT("[SEND USER LIST]\n");

			int type = SEND_USER_LIST;				// 4
			int dataLength = 0;						// 4
			int userListCount = userCount - 1;		// 4

			//dataLength += sizeof(type);
			//dataLength += sizeof(dataLength);
			dataLength += sizeof(userListCount);


			DEBUG_PRINT("GET TOTAL DATA LENGTH\n");
			for (int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if (client[i] != NULL && FD_ISSET(client[i]->fd, &read_fds))
				{
					if (client[i]->fd == focus_fd) // 포커스 된 자신의 화면은 제외하고
					{
						continue;
					}

					int userIdLength = 0;
					userIdLength = sizeof(client[i]->userID);
					dataLength += sizeof(userIdLength);

					int userNameLength = 0;
					userNameLength = client[i]->userNameLength;
					dataLength += sizeof(userNameLength); // HEADER
					dataLength += userNameLength; // VALUE

					int imageLength = 0;
					imageLength = client[i]->jpegOffset;
					dataLength += sizeof(imageLength); // HEADER
					dataLength += imageLength; // VALUE
				}
			}


			int sendOffset = 0;
			char *sendBuf = new char[HEADER_SIZE + dataLength];

			// header
			memcpy(sendBuf, &type, sizeof(type));
			sendOffset += 4;
			memcpy(sendBuf + sendOffset, &dataLength, sizeof(dataLength));
			sendOffset += 4;

			// data
			memcpy(sendBuf + sendOffset, &userListCount, sizeof(userListCount));
			sendOffset += 4;

			for (int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if (client[i] != NULL && FD_ISSET(client[i]->fd, &read_fds))
				{
					if (client[i]->fd == focus_fd) // 포커스 된 자신의 화면은 제외하고
					{
						continue;
					}
					memcpy(sendBuf + sendOffset, &client[i]->userID, sizeof(client[i]->userID));
					sendOffset += 4;

					memcpy(sendBuf + sendOffset, &client[i]->userNameLength, sizeof(client[i]->userNameLength));
					sendOffset += 4;
					memcpy(sendBuf + sendOffset, client[i]->userName, client[i]->userNameLength);
					sendOffset += client[i]->userNameLength;

					memcpy(sendBuf + sendOffset, &client[i]->jpegOffset, sizeof(client[i]->jpegOffset));
					sendOffset += 4;

					memcpy(sendBuf + sendOffset, client[i]->jpegBuffer, client[i]->jpegOffset);
					sendOffset += client[i]->jpegOffset;
				}
			}


			DEBUG_PRINT("%d - %d - %d\n", sendOffset, HEADER_SIZE, dataLength);

			int sndBufRemain = sendOffset;
			while (sndBufRemain > 0)
			{
				int nwrite = write(focus_fd, sendBuf + sendOffset - sndBufRemain, sndBufRemain);
				sndBufRemain -= sndBufRemain;
			}
		}

		sleep(5);
	}

}


// 사용자 포커스 변경 요청을 전송한다.
void *focusObserver(void *id)
{
	while (1)
	{
		// 이전 포커스 ID와 다른지 확인한다.
		// 만약 다르다면 해당 유저에게 포커스를 넘겨주는 통신을 시도한다.
		// 만약 해당 유저를 찾지 못한다면, 사용자중 가장 빠른 소켓 번호의 사용자에게 권한을 넘겨주고 
		
		if (userCount == 0)
		{
			if (waitScreenSize)
			{
				//memcpy(fbp, waitScreen, 1024 * 768 * 3);
			}
			else
			{
				//bzero(fbp, vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8);
			}
		}

		if (focus_fd != ready_fd)
		{
			DEBUG_PRINT("FOCUS CHANGED\n");
			if (FD_ISSET(ready_fd, &read_fds))
			{
				DEBUG_PRINT("SEND FOCUS STATUS!\n");
				int type = CHANGE_FOCUS_STATUS;
				int len = 4;
				int value = 1;
				char *buf = new char[HEADER_SIZE + 4];
				memcpy(buf, &type, sizeof(int));
				memcpy(buf + 4, &len, sizeof(int));
				memcpy(buf + 8, &value, sizeof(int));

				int totalSize = HEADER_SIZE + 4;
				int writeRemain = HEADER_SIZE + 4;
				while (writeRemain > 0)
				{
					int written = write(ready_fd, buf + totalSize - writeRemain, writeRemain);
					writeRemain -= written;
				}

				focus_fd = ready_fd;
				delete buf;
			}
			else // 찾지 못했을 경우
			{
				if (userCount == 0)
				{
					// 화면을 초기화 한다.
					DEBUG_PRINT("cannot find\n");
				}
				else
				{
					DEBUG_PRINT("FIND NEW FOCUS USERS\n");
					// 다른 사용자들 중 자신을 제외한 가장 빠른 소켓을 찾는다.
					for (int i = 0; i < MAX_CLIENT_NUM; i++)
					{
						if (client[i] != NULL && FD_ISSET(client[i]->fd, &read_fds))
						{
							if (client[i]->fd == focus_fd) // 포커스 된 자신의 화면은 건너뛰고
							{
								DEBUG_PRINT("MY FD %d, focus_fd %d\n", client[i]->fd, focus_fd);
								continue;
							}
							
							DEBUG_PRINT("SEND FOCUS STATUS!\n");
							int type = CHANGE_FOCUS_STATUS;
							int len = 4;
							int value = 1;
							char *buf = new char[HEADER_SIZE + 4];
							memcpy(buf, &type, sizeof(int));
							memcpy(buf + 4, &len, sizeof(int));
							memcpy(buf + 8, &value, sizeof(int));

							int totalSize = HEADER_SIZE + 4;
							int writeRemain = HEADER_SIZE + 4;
							while (writeRemain > 0)
							{
								int written = write(client[i]->fd, buf + totalSize - writeRemain, writeRemain);
								writeRemain -= written;
							}

							focus_fd = client[i]->fd;
							ready_fd = focus_fd;

							delete buf;

							break;
						}
					}

				}
			}
		}
	}
}

int getAddress(sockaddr_in sockaddr)
{
	char addressName[INET_ADDRSTRLEN];
	char last[4];
	int lastNum = 0;

	if (inet_ntop(AF_INET, &sockaddr.sin_addr.s_addr, addressName, sizeof(addressName)) != NULL){
		//DEBUG_PRINT("%s%c%d", addressName, '/', ntohs(client_addr.sin_port));
		strncpy(last, addressName + 11, 4);
		lastNum = atoi(last);
		DEBUG_PRINT("IP Last number is %d\n", lastNum);
	}
	else {
		DEBUG_PRINT("Unable to get address\n"); // i just fixed this to printf .. i had it as print before
	}

	return lastNum;
}

int getMyAddress()
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		DEBUG_PRINT("getifaddrs");
		exit(EXIT_FAILURE);
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		// if wlan0
		if ((strcmp(ifa->ifa_name, "eth0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
		{
			if (s != 0)
			{
				DEBUG_PRINT("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			printf("\tInterface : <%s>\n", ifa->ifa_name);
			printf("\t  Address : <%s>\n", host);
		}

		if ((strcmp(ifa->ifa_name, "wlan0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
		{
			if (s != 0)
			{
				DEBUG_PRINT("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			printf("\tInterface : <%s>\n", ifa->ifa_name);
			printf("\t  Address : <%s>\n", host);
		}
	}

	freeifaddrs(ifaddr);
}
// application entry point
int main(int argc, char* argv[])
{
	setFrameBuffer();

	DEBUG_PRINT("Create Socket\n");
	/* Create Socket */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_PRINT("socket error : ");
		exit(1);
	}

	/* Make Server Option */
	clen = sizeof(client_addr);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(3317);

	int optvalue = TRUE;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));

	int snd_size = 0;
	int rcv_size = 0;
	socklen_t optlen;

	if (getsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen) == 0)
	{
		DEBUG_PRINT("Send Buffer Size : %d KB (%d) \n", snd_size / 1024, snd_size);
	}

	if (getsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen) == 0)
	{
		DEBUG_PRINT("Receive Buffer Size : %d KB (%d) \n", rcv_size / 1024, rcv_size);
	}

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		DEBUG_PRINT("bind error : ");
		exit(1);
	}

	FILE *in = fopen("raspberry-pi.jpg", "rb");
	if (in != NULL)
	{
		printf("Wait Screen!\n");
		fseek(in, 0, SEEK_END);
		int waitScreenSize = ftell(in);
		fseek(in, 0, SEEK_SET);

		unsigned char *waitScreenJPEG = new unsigned char[waitScreenSize];
		fread(waitScreenJPEG, sizeof(char), waitScreenSize, in);
		tjhandle tjh = tjInitDecompress();
		int width, height, subsmp;
		tjDecompressHeader2(tjh, waitScreenJPEG, waitScreenSize, &width, &height, &subsmp);
		waitScreen = new char[width * height * 3];
		tjDecompress2(tjh, waitScreenJPEG, waitScreenSize, (unsigned char*)waitScreen, width, 0, height, TJPF_BGR, TJFLAG_FASTDCT);
		tjDestroy(tjh);
	}

	getMyAddress();
	DEBUG_PRINT("Server is open\n");
	/* listening */
	if (listen(server_socket, 5) < 0) {
		DEBUG_PRINT("listen error : ");
		exit(1);
	}

	/* Set File Descriptor */
	FD_ZERO(&read_fds);
	FD_SET(server_socket, &read_fds);

	int fd;
	char chunk[MAX_BUF];

	DEBUG_PRINT("Server is running.\n");

	int err;
	pthread_t btid;
	pthread_create(&btid, NULL, &userListThread, (void *)btid);

	pthread_t fcid;
	pthread_create(&fcid, NULL, &focusObserver, (void *)fcid);
	
	while (1)
	{
		tmp_fds = read_fds;

		if (select(MAX_CLIENT_NUM, &tmp_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 1) {
			DEBUG_PRINT("select error : ");
			exit(1);
		}

		for (fd = 0; fd < MAX_CLIENT_NUM; fd++)
		{
			if (FD_ISSET(fd, &tmp_fds)) {

				if (fd == server_socket)
				{
					client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &clen);
					setSocketOption(client_socket);
					FD_SET(client_socket, &read_fds);

					int lastip = getAddress(client_addr);
					client[client_socket] = new Client(client_socket, lastip);

					DEBUG_PRINT("\nnew client %d descriptor accepted, usercount = %d\n", client_socket, userCount);
				}
				else
				{
					data_len = read(fd, chunk, MAX_BUF);

					if (data_len > 0)
					{
						int ret = client[fd]->receive(chunk, data_len); // client[fd]의 버퍼에 넣는다. 
						if (ret < 0)
						{
							shutdown(fd, SHUT_RDWR);
							close(fd);
							FD_CLR(fd, &read_fds);

							delete client[fd];
							client[fd] = NULL;
							DEBUG_PRINT("socket force shutdown %d\n", fd);
						}
						bzero(chunk, data_len);
					}
					else if (data_len <= 0)
					{
						shutdown(fd, SHUT_RDWR);
						close(fd);
						FD_CLR(fd, &read_fds);

						delete client[fd];
						client[fd] = NULL;
						DEBUG_PRINT("\nclose client at %d\n", fd);
					}
				}
			}
		}
	}

	DEBUG_PRINT("Server shutdown\n");
	unsetFrameBuffer();

	return 0;
}

