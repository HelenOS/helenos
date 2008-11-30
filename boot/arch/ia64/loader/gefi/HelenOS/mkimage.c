#include<stdio.h>
#include<stdlib.h>

int main(int argc,char** argv)
{
	FILE *fi,*fo;
	int count=0;
	int ch;
	fi=fopen("image.bin","rb");
	fo=fopen("image.c","wb");
	fprintf(fo,"char HOSimage[]={\n");
	if((ch=getc(fi))!=EOF) {fprintf(fo,"0x%02X",ch);count++;}
	while((ch=getc(fi))!=EOF) {fprintf(fo,",0x%02X",ch);count++;}
	fprintf(fo,"};\nint HOSimagesize=%d;\n",count);
	return EXIT_SUCCESS;
}

