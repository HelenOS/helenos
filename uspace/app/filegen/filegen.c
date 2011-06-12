
/** @addtogroup filecheck
 * @{
 */

/**
 * @file	filecrc.c
 * @brief	Tool for generating file with random data
 *
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include "crc32.h"

#define NAME	"filegen"
#define VERSION "0.0.1"

#define BUFFERSIZE 256


static void print_help(void);


int main(int argc, char **argv) {
	int rc;
	uint64_t size = 0;
	
	if (argc < 3) {
		print_help();
		return 0;
	}
	
	int fd = open(argv[1], O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("Unable to open %s for writing\n", argv[1]);
		return 1;
	}
	
	rc = str_uint64(argv[2], NULL, 10, true, &size);
	if (rc != EOK) {
		printf("Cannot convert size to number\n");
		return 1;
	}
	
	struct timeval tv;	
	gettimeofday(&tv, NULL);
	srandom(tv.tv_sec + tv.tv_usec / 100000);	

	uint64_t i=0, pbuf=0;
	uint32_t crc=~0;
	char buf[BUFFERSIZE];
	
	while (i<size) {
		pbuf=0;
		while (i<size && pbuf<BUFFERSIZE) {
			buf[pbuf] = rand() % 255;
			i++;
			pbuf++;
		}
		if (pbuf) {
			crc32(buf, pbuf, &crc);
			write(fd, buf, pbuf);
		}
	}
	
	close(fd);
	crc = ~crc;
	printf("%s: %x\n", argv[1], crc);

	return 0;
}


/* Displays help for filegen */
static void print_help(void)
{
	printf(
	   "Usage:  %s <file> <size in bytes>\n",
	   NAME);
}


/**
 * @}
 */
