#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

// default framebuffer palette
typedef enum {
	BLACK			=	0,
	BLUE			=	1,
	GREEN			=	2,
	CYAN			=	3,
	RED				=	4,
	PURPLE			=	5,
	ORANGE			= 	6,
	LTGREY			=	7,
	GREY			=	8,
	LIGHT_BLUE		=	9,
	LIGHT_GREEN		=	10,
	LIGHT_CYAN		=	11,
	LIGHT_RED		=	12,
	LIGHT_PURPLE	=	13,
	YELLOW			=	14,
	WHITE			=	15,

} COLOR_INDEX_T;

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

static unsigned short def_r[] = 
	{	0,	0,	0,	0,	172, 172, 172, 172,
	 	84, 84,	84,	84,	255, 255, 255, 255 };

static unsigned short def_g[] = 
	{	0, 0, 172, 172, 0, 0, 84, 172,
		84,	84, 255, 255, 84, 84, 255, 255 };

static unsigned short def_b[] = 
	{	0, 172, 0, 172, 0, 172, 0, 172,
		84, 255, 84, 255, 84, 255, 84, 255 };

// 'global' variables to store screen info
char *fbp = 0;						// FRAME Buffer Mapped Memory
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

// Loaded Image Buffer
char *buffer = NULL;

// helper function to 'plot' a pixel in given color
void put_pixel(int x, int y, int c)
{
	// calculate the pixel's byte offset inside the buffer
	unsigned int pix_offset = x + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	*((char*)(fbp + pix_offset)) = c; 
}


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

// RGB565 (rrrrrggggggbbbbb) total 16bit
void put_pixel_RGB565(int x, int y, int r, int g, int b)
{
	// calculate the pixels' byte offset inside the buffer
	// note: x * 2 as every pixel is 2 consecutive bytes
	unsigned int pix_offset = x * 2 + y * finfo.line_length;

	// now this is about the same as 'fbp[pix_offset] = value'
	// but a bit more complicated for RGB565
	unsigned short c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
	// or: c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
	// write 'two bytes at once'
	*((unsigned short*)(fbp + pix_offset)) = c;
}

// helper function for drawing - no more need to go mess with
// the main function when just want to change what to draw
void draw() {

	int bufferPos = 0;
	int r, g, b;

	int x, y;
	for (y = 0; y < vinfo.yres/2; y++) {
		for (x=0; x < vinfo.xres; x++) {
			
			// color based on the 16th of the screen width
			//int c = 16 * x / vinfo.xres;
			// Get color from image memory

			// call the helper function
			if ( vinfo.bits_per_pixel == 8 ) {
				// read 1 byte from buffer
				//put_pixel(x, y, c);
			}
			else if( vinfo.bits_per_pixel == 16) {
				// read 2 bytes from buffer
				//put_pixel_RGB565(x, y, def_r[c], def_g[c], def_b[c]);
			}
			else if( vinfo.bits_per_pixel == 24) {
				// read 3 bytes from buffer
				//put_pixel_RGB24(x, y, def_r[c], def_g[c], def_b[c]);
				r = (int)((char *)bufferPos);
				g = (int)((char *)bufferPos + 1);
				b = (int)((char *)bufferPos + 2);
				printf("%d %d %d\n", r, g, b);
				bufferPos += 3;
			}
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

/*
void writeBMP(RGB **Matrix, HEADER head, FILE* arq){
	FILE* out;
	int i,j;
	RGB tmp;
	long pos = 51;

	char header[54];
	fseek(arq,0,0);
	fread(header,54,1,arq);
	out = fopen("out.bmp","w");

	fseek(out,0,0);
	fwrite(header,54,1,out);

	printf("\nMatrix = %c\n",Matrix[0][0].RGB[0]);
	for(i=0;i<height;i++){
		for(j=0;j<width;j++){
			pos+= 3;
			fseek(out,pos,0);
			tmp = Matrix[i][j];
			fwrite(&tmp,(sizeof(RGB)),1,out);
		}
	}
	fflush(out);
	fclose(out);
}

unsigned char* readBMP(char* filename)
{
    int i;
    FILE* f = fopen(filename, "rb");
    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

    // extract image height and width from header
    int width = *(int*)&info[18];
    int height = *(int*)&info[22];

    int size = 3 * width * height;
    unsigned char* data = new unsigned char[size]; // allocate 3 bytes per pixel
    fread(data, sizeof(unsigned char), size, f); // read the rest of the data at once
    fclose(f);

    for(i = 0; i < size; i += 3)
    {
            unsigned char tmp = data[i];
            data[i] = data[i+2];
            data[i+2] = tmp;
    }

    return data;
}
*/

// application entry point
int main(int argc, char* argv[])
{
	int fbfd = 0; // framebuffer filedescriptor
	struct fb_var_screeninfo orig_vinfo;
	long int screensize = 0;

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

	if(argc == 3)
	{
		if(strcmp(argv[2], "-depth")==0)
		{
			printf("-depth set\n");
			vinfo.bits_per_pixel = atoi(argv[3]);		
		}
	}

	// Change variable info
	// vinfo.bits_per_pixel = 8;
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

	char name[15];
	printf("Type the image's name : ");
	scanf("%s",name);
	// Read bitmap
	FILE *in = fopen(name, "r");
	if(!in)
	{
		printf("File Open Error\n");
		return 1;
	}
	else
	{
		fseek(in, 0, SEEK_END);
		printf("File size is %d\n", ftell(in));
	}

	// Get Bitmap Info
	INFOHEADER info;
	info = readInfo(in);

	int height, width, bpp;
	height = info.height;
	width = info.width;
	bpp = info.bpp;

	printf("File width %d, height %d, bpp %d\n", width, height, bpp);

	//buffer = (char *)malloc(1920 * 1080 * 3);
	buffer = (char *)malloc(1920 * 1080 * 3);
	if(buffer == NULL) {
		printf("Create Buffer Failed");
	}

	fread(buffer, bpp/3, width * height, in);

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
		draw();
		sleep(5);
	}

	// cleanup
	munmap(fbp, screensize);
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		printf("Error re-setting variable information.\n");
	}

	close(fbfd);

	return 0;
}


