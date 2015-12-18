#include <stdio.h>
#include <string.h>

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

#pragma once
class Bitmap
{
public:
	Bitmap();
	~Bitmap();
};

int isBMP(FILE* arq, INFOHEADER info);
INFOHEADER readInfo(FILE* arq);