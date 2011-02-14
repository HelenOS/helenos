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
static void print_superblock(ext2_superblock_t *sb);

int main(int argc, char **argv)
{

	int rc;
	char *dev_path;
	devmap_handle_t handle;
	ext2_filesystem_t filesystem;
	
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

	rc = ext2_filesystem_init(&filesystem, handle);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libext2.\n");
		return 3;
	}
	
	rc = ext2_filesystem_check_sanity(&filesystem);
	if (rc != EOK) {
		printf(NAME ": Filesystem did not pass sanity check.\n");
		return 3;
	}
	
	print_superblock(filesystem.superblock);

	ext2_filesystem_fini(&filesystem);

	return 0;
}


static void syntax_print(void)
{
	printf("syntax: ext2info <device_name>\n");
}

static void print_superblock(ext2_superblock_t *superblock)
{
	uint16_t magic;
	uint32_t first_block;
	uint32_t block_size;
	uint32_t fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t rev_major;
	uint16_t rev_minor;
	uint16_t state;
	uint32_t first_inode;
	uint16_t inode_size;
	uint32_t total_blocks;
	uint32_t reserved_blocks;
	uint32_t free_blocks;
	uint32_t total_inodes;
	uint32_t free_inodes;	
	uint32_t os;
	
	int pos;
	unsigned char c;
	
	magic = ext2_superblock_get_magic(superblock);
	first_block = ext2_superblock_get_first_block(superblock);
	block_size = ext2_superblock_get_block_size(superblock);
	fragment_size = ext2_superblock_get_fragment_size(superblock);
	blocks_per_group = ext2_superblock_get_blocks_per_group(superblock);
	fragments_per_group = ext2_superblock_get_fragments_per_group(superblock);
	rev_major = ext2_superblock_get_rev_major(superblock);
	rev_minor = ext2_superblock_get_rev_minor(superblock);
	state = ext2_superblock_get_state(superblock);
	first_inode = ext2_superblock_get_first_inode(superblock);
	inode_size = ext2_superblock_get_inode_size(superblock);
	total_blocks = ext2_superblock_get_total_block_count(superblock);
	reserved_blocks = ext2_superblock_get_reserved_block_count(superblock);
	free_blocks = ext2_superblock_get_free_block_count(superblock);
	total_inodes = ext2_superblock_get_total_inode_count(superblock);
	free_inodes = ext2_superblock_get_free_inode_count(superblock);
	os = ext2_superblock_get_os(superblock);
	
	printf("Superblock:\n");
	
	if (magic == EXT2_SUPERBLOCK_MAGIC) {
		printf("  Magic value: %X (correct)\n", magic);
	}
	else {
		printf("  Magic value: %X (incorrect)\n", magic);
	}
	
	printf("  Revision: %u.%hu\n", rev_major, rev_minor);
	printf("  State: %hu\n", state);
	printf("  Creator OS: %u\n", os);
	printf("  First block: %u\n", first_block);
	printf("  Block size: %u bytes (%u KiB)\n", block_size, block_size/1024);
	printf("  Blocks per group: %u\n", blocks_per_group);
	printf("  Total blocks: %u\n", total_blocks);
	printf("  Reserved blocks: %u\n", reserved_blocks);
	printf("  Free blocks: %u\n", free_blocks);
	printf("  Fragment size: %u bytes (%u KiB)\n", fragment_size,
	    fragment_size/1024);
	printf("  Fragments per group: %u\n", fragments_per_group);
	printf("  First inode: %u\n", first_inode);
	printf("  Inode size: %hu bytes\n", inode_size);
	printf("  Total inodes: %u\n", total_inodes);
	printf("  Free inodes: %u\n", free_inodes);
	
	
	if (rev_major == 1) {
		printf("  UUID: ");
		for (pos = 0; pos < 16; pos++) {
			printf("%02x", superblock->uuid[pos]);
		}
		printf("\n");
		
		printf("  Volume label: ");
		for (pos = 0; pos < 16; pos++) {
			c = superblock->volume_name[pos];
			if (c >= 32 && c < 128) {
				putchar(c);
			}
			else {
				putchar(' ');
			}
		}
		printf("\n");
	}
	
}

/**
 * @}
 */
