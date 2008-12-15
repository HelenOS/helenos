#include "font-8x16.h"

#define FB_REG "r8"
#define SCAN_REG "r9"

#define ADDR_REG "r10"
#define FG_REG "r11"
#define BG_REG "r12"

#define FG_COLOR 0xffffffff
#define BG_COLOR 0x00000000

#define BPP 4

void print_macro_init(void) {
	printf(".macro DEBUG_INIT\n");
	printf("#ifdef CONFIG_DEBUG\n");
	printf("\tlis %s, %d\n", FG_REG, FG_COLOR >> 16);
	printf("\tori %s, %s, %d\n", FG_REG, FG_REG, FG_COLOR & 0xffff);
	printf("\t\n");
	printf("\tlis %s, %d\n", BG_REG, BG_COLOR >> 16);
	printf("\tori %s, %s, %d\n", BG_REG, BG_REG, BG_COLOR & 0xffff);
	printf("\t\n");
	printf("\tmr %s, %s\n", ADDR_REG, FB_REG);
	printf("#endif\n");
	printf(".endm\n");
}

void print_macro(const char *name) {
	printf(".macro DEBUG_%s\n", name);
	printf("#ifdef CONFIG_DEBUG\n");
	
	unsigned int y;
	for (y = 0; y < FONT_SCANLINES; y++) {
		printf("\t\n");
		
		if (y > 0)
			printf("\tadd %s, %s, %s\n", ADDR_REG, ADDR_REG, SCAN_REG);
		
		unsigned int i;
		for (i = 0; name[i] != 0; i++) {
			char c = name[i];
			
			unsigned int x;
			for (x = 0; x < FONT_WIDTH; x++) {
				if (((fb_font[c * FONT_SCANLINES + y] >> (FONT_WIDTH - x)) & 1) == 1)
					printf("\tstw %s, %d(%s)\n", FG_REG, (i * FONT_WIDTH + x) * BPP, ADDR_REG);
				else
					printf("\tstw %s, %d(%s)\n", BG_REG, (i * FONT_WIDTH + x) * BPP, ADDR_REG);
			}
		}
	}
	
	printf("#endif\n");
	printf(".endm\n");
}

int main(int argc, char *argv[]) {
	print_macro_init();
	
	int i;
	for (i = 1; i < argc; i++) {
		printf("\n");
		print_macro(argv[i]);
	}
	
	return 0;
}
