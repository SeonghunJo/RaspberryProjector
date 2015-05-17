#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

// application entry point
int main(int argc, char* argv[])
{
	int fbfd = 0; // framebuffer filedescriptor
	struct fb_var_screeninfo var_info;
	struct fb_fix_screeninfo fix_info;
	
	long int screensize = 0;
	char *fbp = 0;

	// Open the framebuffer device file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		printf("Error: cannot open framebuffer device.\n");
		return(1);
	}

	printf("The framebuffer device opened.\n");

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var_info)) {
		printf("Error reading variable screen info.\n");
	}

	// GET fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info)) {
		printf("Error reading fixed screen info.\n");	
	}


	printf("Display info %dx%d, %d bpp\n",
			var_info.xres, var_info.yres,
			var_info.bits_per_pixel );

	// map framebuffer to user memory
	screensize = fix_info.smem_len;
	
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
		// just fill upper half of the screen with something
		memset(fbp, 0xff, screensize/2);
		// and lower half with something else
		memset(fbp + screensize/2, 0x18, screensize/2);
	}

	// cleanup
	munmap(fbp, screensize);
	close(fbfd);

	return 0;
}
