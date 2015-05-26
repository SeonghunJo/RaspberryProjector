#include <stdio.h>
#include <string>
#include "jpeglib.h"

using namespace std;

class JPEGConverter
{
    int w, h, d;

    int out_h;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

public:

    JPEGConverter()
    {

    }

    void init()
    {
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
    }

    void read()
    {

        //jpeg_stdio_src(&cinfo, fp);
        jpeg_read_header(&cinfo, true);
    }
};

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

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage) .jpeg_test filename\n");
		return 0;
	}

	string fn = argv[1];
	if(fn.empty())
	{
		printf("empty filename\n");
		return 0;
	}

	FILE* fp = fopen(fn.c_str(), "rb");
	if(fp == NULL)
	{
		printf("failed to open %s\n", fn.c_str());
		return 0;
	}

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, true);

    jpeg_start_decompress(&cinfo);

	//cinfo.scale_num = 1;
	//cinfo.scale_denom = 8; // 8분의 1크기로 bitmap을 변환하여 출력한다.

	int w = cinfo.image_width;
	int h = cinfo.image_height;
	int d = cinfo.jpeg_color_space;
	int out_h = cinfo.output_height;

	printf("width:%d height:%d depth:%d out_height:%d\n", w, h, d, out_h);
    printf("output_scanline %d\n", cinfo.output_scanline);

    int data_len = 0;
	unsigned char *data = new unsigned char[w * h * d];
	while(cinfo.output_scanline < cinfo.output_height)
	{
	    printf("read lines %d, ", cinfo.output_scanline);
		jpeg_read_scanlines(&cinfo, &data, 1);
		data += d * cinfo.output_width;
	}

    FILE *outfile = fopen("outjpeg.jpg", "wb");

    fclose(outfile);
	fclose(fp);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}
