/*
 * Copyright (c) 2011 Oleg Romanenko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
