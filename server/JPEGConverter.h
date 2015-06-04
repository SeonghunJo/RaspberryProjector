#pragma once

#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <turbojpeg.h>

typedef struct {
	long filesize;
	char reserved[2];
	long headersize;
	long infoSize;
	long width;
	long depth;
	short biPlanes;
	short bits;
	long biCompression;
	long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	long biClrUsed;
	long biClrImportant;
} BMPHEAD;


class JPEGConverter
{
	/* dimensions of the image we want to write */
	int width;
	int height;
	int bytes_per_pixel;   /* or 1 for GRACYSCALE images */
	int color_space; /* or JCS_GRAYSCALE for grayscale images */

	/* we will be using this uninitialized pointer later to store raw, uncompressd image */
	unsigned char *raw_image;

public:
	JPEGConverter();
	~JPEGConverter();

	int decode_frame(const char *buffer, int length, void *samples, int *w = NULL, int *h = NULL);
	int decode_frame2(const char *buffer, int length, unsigned char *samples);
	int write_bmp_file(char *filename);
	int read_jpeg_file(char *filename);
};
