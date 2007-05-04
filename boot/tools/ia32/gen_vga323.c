#include <stdio.h>

#define RED(i) ((i >> 5) & ((1 << 3) - 1))
#define GREEN(i) ((i >> 3) & ((1 << 2) - 1))
#define BLUE(i) (i & ((1 << 3) - 1))

int main(int argc, char *argv[]) {
	unsigned int i;
	
	for (i = 0; i < 256; i++)
		printf("\t.byte 0x%02x, 0x%02x, 0x%02x, 0x00\n", BLUE(i) * 9, GREEN(i) * 21, RED(i) * 9);
	
	return 0;
}
