/*
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup fs
 * @{
 */

/**
 * @file	ext2info.c
 * @brief	Tool for displaying information about ext2 filesystem
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libblock.h>
#include <mem.h>
#include <devmap.h>
#include <byteorder.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <errno.h>
#include <libext2.h>

#define NAME	"ext2info"

static void syntax_print(void);

int main(int argc, char **argv)
{

	int rc;
	char *dev_path;
	devmap_handle_t handle;
	ext2_superblock_t *superblock;
	
	uint16_t magic;
	
	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc; ++argv;

	if (argc != 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}

	dev_path = *argv;

	rc = devmap_device_get_handle(dev_path, &handle, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(handle, 2048);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = ext2_superblock_read_direct(&superblock, handle);
	if (rc != EOK)  {
		printf(NAME ": Error reading superblock.\n");
		return 3;
	}
	
	printf("Superblock:\n");
	magic = ext2_superblock_get_magic(superblock);
	if (magic == EXT2_SUPERBLOCK_MAGIC) {
		printf("  Magic value: %X (correct)\n", magic);
	}
	else {
		printf("  Magic value: %X (incorrect)\n", magic);
	}
	
	
	free(superblock);

	block_fini(handle);

	return 0;
}


static void syntax_print(void)
{
	printf("syntax: blkdump [--offset <num_blocks>] [--count <num_blocks>] <device_name>\n");
}

/**
 * @}
 */
