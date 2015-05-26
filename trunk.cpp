

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
