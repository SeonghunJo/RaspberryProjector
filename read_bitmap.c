#include <stdio.h>
#include <stdlib.h>

/* Global */
int height, width;

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
} INFOHEADER;


// ********** Create Matrix **********
RGB** createMatrix(){
    RGB** Matrix;
    int i;
    Matrix = (RGB **) malloc(sizeof (RGB*) * height);
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

// ********** Verify if the file is BMP *********
void isBMP(FILE* arq, HEADER head, INFOHEADER info)
{
    char type[3];
    unsigned short int bpp;
    fseek(arq,0,0);
    fread(type,1,2,arq);
    type[2] = '\0';
    
    fseek(arq,28,0);
    fread(&bpp,1,2,arq);
    
    if (strcmp(type,"BM") || (bpp != 24)){
        printf("\nThe file is not BMP format or is not 24 bits\n");
		printf("bpp : %d", bpp);
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

RGB** loadImage(FILE* arq, RGB** Matrix){
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
    return(Matrix);
}

void main(int argc, char* argv[])
{
    /* in your main program you just call */
    FILE* arq; /* the bitmap file 24 bits */
    RGB** Matrix_aux, Matrix;
    INFOHEADER info;

	if(argc != 3)
		return 0;
	else
		

    info = readInfo(FILE* arq);
    height = info.height;
    width = info.width;
    
	printf("height %d, width %d\n", height, width);
    Matrix_aux = createMatrix();
    Matrix = loadImage(arq,Matrix_aux);
}
