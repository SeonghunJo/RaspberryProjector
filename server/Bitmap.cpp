#include "Bitmap.h"


Bitmap::Bitmap()
{
}


Bitmap::~Bitmap()
{
}


// ********** Verify if the file is BMP *********
int isBMP(FILE* arq, INFOHEADER info){
	char type[3];
	unsigned short int bpp;
	fseek(arq, 0, 0);
	fread(type, 1, 2, arq);
	type[2] = '\0';

	fseek(arq, 28, 0);
	fread(&bpp, 1, 2, arq);

	printf("Bitmap Type : %s\n", type);
	printf("Bitmap BPP : %d\n", bpp);

	if (strcmp(type, "BM") || (bpp != 24)){
		printf("\nThe file is not BMP format or is not 24 bits\n");
		return 1;
	}
}

// ********** Read BMP info from file **********
INFOHEADER readInfo(FILE* arq)
{
	INFOHEADER info;

	// Image Width in pixels
	fseek(arq, 18, 0);
	fread(&info.width, 1, 4, arq);

	// Image Height in pixels
	fseek(arq, 22, 0);
	fread(&info.height, 1, 4, arq);

	// Color depth, BPP (bits per pixel)
	fseek(arq, 28, 0);
	fread(&info.bpp, 1, 2, arq);

	// Compression type
	// 0 = Normmally
	// 1 = 8 bits per pixel
	// 2 = 4 bits per pixel
	fseek(arq, 30, 0);
	fread(&info.compression, 1, 4, arq);

	// Image size in bytes
	fseek(arq, 34, 0);
	fread(&info.imagesize, 1, 4, arq);

	// Number of color used (NCL)
	// value = 0 for full color set
	fseek(arq, 46, 0);
	fread(&info.colours, 1, 4, arq);

	// Number of important color (NIC)
	// value = 0 means all collors important
	fseek(arq, 50, 0);
	fread(&info.impcolours, 1, 4, arq);

	return(info);
}

