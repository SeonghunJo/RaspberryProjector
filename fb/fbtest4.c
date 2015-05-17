#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

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

// helper function for drawing - no more need to go mess with
// the main function when just want to change what to draw
void draw() {

	int x, y;

	for (y = 0; y < (vinfo.yres / 2); y++) {
		for (x=0; x < vinfo.xres; x++) {
			
			// color based on the 16th of the screen width
			int c = 16 * x / vinfo.xres;

			// call the helper function
			put_pixel(x, y, c);
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
	screensize = finfo.smem_len;
	
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


