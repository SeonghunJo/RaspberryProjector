#include "JPEGConverter.h"


JPEGConverter::JPEGConverter()
{
	int x, y;
	x = y = 0;
}


JPEGConverter::~JPEGConverter()
{
	if (raw_image != NULL)
		free(raw_image);
}

int JPEGConverter::decode_frame(const char *buffer, int length, void *samples, int *w, int *h)
{
	int n_samples;
	struct jpeg_error_mgr err;
	struct jpeg_decompress_struct cinfo = { 0 };

	clock_t start, finish;
	double elapsed;

	/* create decompressor */
	jpeg_create_decompress(&cinfo);
	cinfo.err = jpeg_std_error(&err);
	cinfo.do_fancy_upsampling = FALSE;
	//cinfo.do_block_smoothing = FALSE;
	//cinfo.dct_method = JDCT_FASTEST;
	//cinfo.dither_mode = JDITHER_ORDERED;

	/* set source buffer */
	jpeg_mem_src(&cinfo, (unsigned char *)buffer, length);

	/* read jpeg header */
	jpeg_read_header(&cinfo, 1);

	/*
	printf("JPEG File Information: \n");
	printf("Image width and height: %d pixels and %d pixels.\n", width = cinfo.image_width, height = cinfo.image_height);
	printf("Color components per pixel: %d.\n", bytes_per_pixel = cinfo.num_components);
	printf("Color space: %d.\n", cinfo.jpeg_color_space);
	*/
	if (w != NULL)
		*w = cinfo.image_width;
	if (h != NULL)
		*h = cinfo.image_height;

	/* decompress */
	jpeg_start_decompress(&cinfo);

	/*
	if (samples == NULL)
	{
	printf("Create Sample buffer\n");
	samples = (unsigned char *)calloc(sizeof(unsigned char), cinfo.output_width*cinfo.output_height*cinfo.num_components);
	if (samples == NULL)
	{
	printf("Allocation Failed\n");
	}
	}
	*/
	start = clock();
	/* now actually read the jpeg into the raw buffer */
	/* libjpeg data structure for storing one row, that is, scanline of an image */
	JSAMPROW row_pointer[1];
	row_pointer[0] = (unsigned char *)malloc(cinfo.output_width*cinfo.num_components);

	int offset = 0;
	/* read scanlines */
	while (cinfo.output_scanline < cinfo.output_height)
	{
		//n_samples = jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&samples, 1);
		n_samples = jpeg_read_scanlines(&cinfo, row_pointer, 1);
		//samples += n_samples * cinfo.image_width * cinfo.num_components;
		memcpy(samples + offset, row_pointer[0], cinfo.image_width * cinfo.num_components);
		offset += n_samples * cinfo.image_width * cinfo.num_components;
	}

	finish = clock();
	printf("[Times %0.3f]\n", (double)(finish - start) / CLOCKS_PER_SEC);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(row_pointer[0]);
	return offset;
}

int JPEGConverter::decode_frame2(const char *buffer, int length, unsigned char *samples)
{
	long unsigned int _jpegSize = length; //!< _jpegSize from above
	unsigned char* _compressedImage = (unsigned char *)buffer; //!< _compressedImage from above

	int jpegSubsamp, width, height;
	//unsigned char buffer[width*height*COLOR_COMPONENTS]; //!< will contain the decompressed image

	tjhandle _jpegDecompressor = tjInitDecompress();

	tjDecompressHeader2(_jpegDecompressor, _compressedImage, _jpegSize, &width, &height, &jpegSubsamp);
	
	//tjDecompress2(_jpegDecompressor, _compressedImage, _jpegSize, samples, width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT);
	tjDecompress2(_jpegDecompressor, _compressedImage, _jpegSize, samples, width, 0/*pitch*/, height, TJPF_BGR, TJFLAG_FASTDCT);

	tjDestroy(_jpegDecompressor);

	return width*height*3;
}


int JPEGConverter::write_bmp_file(char *filename)
{
	BMPHEAD bh;
	memset((char *)&bh, 0, sizeof(BMPHEAD)); /* sets everything to 0 */

	//bh.filesize  =   calculated size of your file (see below)
	//bh.reserved  = two zero bytes
	bh.headersize = 54L;//  (for 24 bit images)
	bh.infoSize = 0x28L;//  (for 24 bit images)
	bh.width = width;//in pixels of your image
	bh.depth = height;// in pixels of your image
	bh.biPlanes = 1;//(for 24 bit images)
	bh.bits = 24;//(for 24 bit images)
	bh.biCompression = 0L;;//  (no compression)

	int bytesPerLine;

	bytesPerLine = width * 3;  /* (for 24 bit images) */
	/* round up to a dword boundary */
	if (bytesPerLine & 0x0003)
	{
		bytesPerLine |= 0x0003;
		++bytesPerLine;
	}
	bh.filesize = bh.headersize + (long)bytesPerLine*bh.depth;

	FILE * bmpfile;

	printf("Bytes per line : %d\n", bytesPerLine);

	bmpfile = fopen(filename, "wb");
	if (bmpfile == NULL)
	{
		printf("Error opening output file\n");
		/* -- close all open files and free any allocated memory -- */
		exit(1);
	}
	fwrite("BM", 1, 2, bmpfile);
	fwrite((char *)&bh, 1, sizeof(bh), bmpfile);

	char *linebuf;

	linebuf = (char *)calloc(1, bytesPerLine);
	if (linebuf == NULL)
	{
		printf("Error allocating memory\n");
		free(raw_image);
		/* -- close all open files and free any allocated memory -- */
		exit(1);
	}


	int line, x;

	for (line = height - 1; line >= 0; line--)
	{
		/* fill line linebuf with the image data for that line */
		for (x = 0; x < width; x++)
		{
			*(linebuf + x*bytes_per_pixel) = *(raw_image + (x + line*width)*bytes_per_pixel + 2);
			*(linebuf + x*bytes_per_pixel + 1) = *(raw_image + (x + line*width)*bytes_per_pixel + 1);
			*(linebuf + x*bytes_per_pixel + 2) = *(raw_image + (x + line*width)*bytes_per_pixel + 0);
		}

		/* remember that the order is BGR and if width is not a multiple
		of 4 then the last few bytes may be unused
		*/
		fwrite(linebuf, 1, bytesPerLine, bmpfile);
	}
	free(linebuf);
	fclose(bmpfile);
}



/**
* read_jpeg_file Reads from a jpeg file on disk specified by filename and saves into the
* raw_image buffer in an uncompressed format.
*
* \returns positive integer if successful, -1 otherwise
* \param *filename char string specifying the file name to read from
*
*/

int JPEGConverter::read_jpeg_file(char *filename)
{
	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* libjpeg data structure for storing one row, that is, scanline of an image */
	JSAMPROW row_pointer[1];

	FILE *infile = fopen(filename, "rb");
	unsigned long location = 0;
	int i = 0;

	if (!infile)
	{
		printf("Error opening jpeg file %s\n!", filename);
		return -1;
	}
	/* here we set up the standard libjpeg error handler */
	cinfo.err = jpeg_std_error(&jerr);
	/* setup decompression process and source, then read JPEG header */
	jpeg_create_decompress(&cinfo);
	/* this makes the library read from infile */
	jpeg_stdio_src(&cinfo, infile);
	/* reading the image header which contains image information */
	jpeg_read_header(&cinfo, TRUE);
	/* Uncomment the following to output image information, if needed. */

	printf("JPEG File Information: \n");
	printf("Image width and height: %d pixels and %d pixels.\n", width = cinfo.image_width, height = cinfo.image_height);
	printf("Color components per pixel: %d.\n", bytes_per_pixel = cinfo.num_components);
	printf("Color space: %d.\n", cinfo.jpeg_color_space);

	/* Start decompression jpeg here */
	jpeg_start_decompress(&cinfo);

	/* allocate memory to hold the uncompressed image */
	raw_image = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
	/* now actually read the jpeg into the raw buffer */
	row_pointer[0] = (unsigned char *)malloc(cinfo.output_width*cinfo.num_components);
	/* read one scan line at a time */
	while (cinfo.output_scanline < cinfo.image_height)
	{
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for (i = 0; i<cinfo.image_width*cinfo.num_components; i++)
			raw_image[location++] = row_pointer[0][i];
	}
	/* wrap up decompression, destroy objects, free pointers and close open files */
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(row_pointer[0]);
	fclose(infile);
	/* yup, we succeeded! */
	return 1;
}

