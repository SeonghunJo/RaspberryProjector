#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
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
char *fbp = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

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

	int x, y;

	for (y = 0; y < (vinfo.yres / 2); y++) {
		for (x=0; x < vinfo.xres; x++) {
			
			// color based on the 16th of the screen width
			int c = 8 * x / vinfo.xres;

			// call the helper function
			if ( vinfo.bits_per_pixel == 8 ) {
				put_pixel(x, y, c);
			}
			else if( vinfo.bits_per_pixel == 16) {
				put_pixel_RGB565(x, y, def_r[c], def_g[c], def_b[c]);
			}
			else if( vinfo.bits_per_pixel == 24) {
				put_pixel_RGB24(x, y, def_r[c], def_g[c], def_b[c]);
			}
		}
	}
}

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

	// Store for rest (copy var_info to vinfo_orig
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

	if(argc == 3)
	{
		if(strcmp(argv[2], "-depth")==0)
		{

		}
	}

	// Change variable info
	vinfo.bits_per_pixel = 8;
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


