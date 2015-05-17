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
	struct fb_var_screeninfo orig_vinfo;
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

	// Store for rest (copy var_info to vinfo_orig
	memcpy(&orig_vinfo, &var_info, sizeof(struct fb_var_screeninfo));

	// Change variable info
	var_info.bits_per_pixel = 8;
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &var_info)) {
		printf("Error setting variable information. \n");
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
		int x, y;
		unsigned int pix_offset;

		for(y = 0; y < (var_info.yres / 2); y++) {
			for(x = 0; x < var_info.xres; x++) {

				// calculate the pixel's byte offset inside the buffer
				// see the image above in the blog...
				pix_offset = x + y * fix_info.line_length;

				// now this is about the same as fbp[pix_offset] = value
				*((char*)(fbp + pix_offset)) = 16 * x / var_info.xres;

			}
		}

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
