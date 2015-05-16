#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//***************** Structures  *******************
typedef struct
	{
		char		type[2]; 	// file type
		unsigned int	size; 		// file size in bytes
		unsigned short int reserved1,reserved2;
		unsigned int	offset; 	// offset to image data
	}HEADER;


typedef struct
        {
                unsigned char RGB[3];
        }RGB;

typedef struct
        {
                unsigned int size;
                int width,height;
                unsigned short int planes;
                unsigned short int bpp;
                unsigned int compression;
                unsigned int imagesize;
                int xresolution,yresolution;
                unsigned int colours;
                unsigned int impcolours;
        }INFOHEADER;


FILE* exist(char *name);
void isBMP(FILE* arq,HEADER head,INFOHEADER info);
INFOHEADER readInfo(FILE* arq);
RGB** createMatrix();
void loadImage(FILE* arq, RGB** Matrix);
void writeBMP(RGB **OutMatrix, HEADER head, FILE* arq);
void freeMatrix(RGB **Matrix,INFOHEADER info);

/* Global */
int height, width;

int main(int argc, char** argv){

	FILE *arq; /* the bitmap file 24 bits */
	RGB  **Matrix;
	INFOHEADER info;
	HEADER head;
	char name[15];

	printf("Type the image's name : ");
	scanf("%s",name);

	arq = exist(name);
	isBMP(arq,head,info);
	info = readInfo(arq);
	height = info.height;
	width = info.width;
	printf("width %d, height %d\n", width, height);

	Matrix = createMatrix(info);
	printf("\nMatrix Created\n");
	loadImage(arq,Matrix);
	printf("\nImage Loaded\n");
	writeBMP(Matrix,head,arq);
	printf("\nIamge Written\n");
	return(0);
}


// ********** Verify if the file exist **********
FILE* exist(char *name)
{
	FILE *tmp;
	tmp = fopen(name,"r+b");
	if (!tmp)
	{
		printf("\nERROR: Incorrect file or not exist!\n");
		exit(0);
	}
	fseek(tmp,0,0);
	return(tmp);
}

// ********** Verify if the file is BMP *********
void isBMP(FILE* arq, HEADER head, INFOHEADER info){
        char type[3];
        unsigned short int bpp;
        fseek(arq,0,0);
        fread(type,1,2,arq);
        type[2] = '\0';

        fseek(arq,28,0);
        fread(&bpp,1,2,arq);
		
		printf("Bitmap Type : %s\n", type);
		printf("Bitmap BPP : %d\n", bpp);

        if (strcmp(type,"BM") || (bpp != 24)){
                printf("\nThe file is not BMP format or is not 24 bits\n");
                exit(0);
        }
}

// ********** Read BMP info from file **********
INFOHEADER readInfo(FILE* arq){
        INFOHEADER info;

        // Image Width in pixels
        fseek(arq,18,0);
        fread(&info.width,1,4,arq);

        // Image Height in pixels
        fseek(arq,22,0);
        fread(&info.height,1,4,arq);

        // Color depth, BPP (bits per pixel)
        fseek(arq,28,0);
        fread(&info.bpp,1,2,arq);

        // Compression type
        // 0 = Normmally
        // 1 = 8 bits per pixel
        // 2 = 4 bits per pixel
        fseek(arq,30,0);
        fread(&info.compression,1,4,arq);

        // Image size in bytes
        fseek(arq,34,0);
        fread(&info.imagesize,1,4,arq);

        // Number of color used (NCL)
        // value = 0 for full color set
        fseek(arq,46,0);
        fread(&info.colours,1,4,arq);

        // Number of important color (NIC)
        // value = 0 means all collors important
        fseek(arq,50,0);
        fread(&info.impcolours,1,4,arq);

        return(info);
}

void loadImage(FILE* arq, RGB** Matrix){
        int i,j;
        RGB tmp;
        long pos = 51;

        fseek(arq,0,0);

        for (i=0; i<height; i++){
                for (j=0; j<width; j++){
                        pos+= 3;
                        fseek(arq,pos,0);
                        fread(&tmp,(sizeof(RGB)),1,arq);
                        Matrix[i][j] = tmp;
                }
        }
}

// ********** Create Matrix **********
RGB** createMatrix(){
        RGB** Matrix;
        int i;
        Matrix = (RGB **) malloc (sizeof (RGB*) * height);
        if (Matrix == NULL){
                perror("***** No memory available *****");
                exit(0);
        }
        for (i=0;i<height;i++){
                Matrix[i] = (RGB *) malloc (sizeof(RGB) * width);
                if (Matrix[i] == NULL){
                perror("***** No memory available *****");
                        exit(0);
                }
        }
        return(Matrix);
}

// ********** Image Output **********
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

// ********** Free memory allocated for Matrix **********
void freeMatrix(RGB** Matrix,INFOHEADER info)
{
	int i;
	int lines = info.height;

	for (i=0;i<lines;i++){
		free(Matrix[i]);
	}
	free(Matrix);
}

