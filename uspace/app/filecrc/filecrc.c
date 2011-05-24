
/** @addtogroup filecheck
 * @{
 */

/**
 * @file	filecrc.c
 * @brief	Tool for calculating CRC32 checksum for a file(s)
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include "crc32.h"

#define NAME	"filecrc"
#define VERSION "0.0.2"

static void print_help(void);


int main(int argc, char **argv) {

	if (argc < 2) {
		print_help();
		return 0;
	}
	
	int i;
	for (i = 1; argv[i] != NULL && i < argc; i++) {
		uint32_t hash = 0;		
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			printf("Unable to open %s\n", argv[i]);
			continue;
		}
		
		if (crc32(fd, &hash) == 0) {
			printf("%s : %x\n", argv[i], hash);
		}
		
		close(fd);
	}

	return 0;
}


/* Displays help for filecrc */
static void print_help(void)
{
	printf(
	   "Usage:  %s <file1> [file2] [...]\n",
	   NAME);
}


/**
 * @}
 */
