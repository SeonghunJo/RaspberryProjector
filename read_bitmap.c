#include <stdio.h>

int main(int argc, char* argv[])
{
	char* filename = "3.bmp";

	FILE *in = fopen(filename, "r");
	
	if(in == NULL)
	{
		printf("Error");
	}
	else
	{
		printf("Open Successs");
	}

	fclose(in);

	return 0;
}
