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
#include <loc.h>
#include <byteorder.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <errno.h>
#include <libext2.h>
#include <assert.h>

#define NAME	"ext2info"

static void syntax_print(void);
static void print_superblock(ext2_superblock_t *);
static void print_block_groups(ext2_filesystem_t *);
static void print_block_group(ext2_block_group_t *);
static void print_inode_by_number(ext2_filesystem_t *, uint32_t, bool, uint32_t,
    bool, bool);
static void print_data(unsigned char *, size_t);
static void print_inode(ext2_filesystem_t *, ext2_inode_t *, bool);
static void print_inode_data(ext2_filesystem_t *, ext2_inode_t *, uint32_t);
static void print_directory_contents(ext2_filesystem_t *, ext2_inode_ref_t *);

#define ARG_SUPERBLOCK 1
#define ARG_BLOCK_GROUPS 2
#define ARG_INODE 4
#define ARG_NO_CHECK 8
#define ARG_INODE_DATA 16
#define ARG_INODE_LIST 32
#define ARG_INODE_BLOCKS 64
#define ARG_COMMON (ARG_SUPERBLOCK)
#define ARG_ALL 127


int main(int argc, char **argv)
{

	int rc;
	char *endptr;
	char *dev_path;
	service_id_t service_id;
	ext2_filesystem_t filesystem;
	int arg_flags;
	uint32_t inode = 0;
	uint32_t inode_data = 0;
	
	arg_flags = 0;
	
	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}
	
	/* Skip program name */
	--argc; ++argv;
	
	if (argc > 0 && str_cmp(*argv, "--no-check") == 0) {
		--argc; ++argv;
		arg_flags |= ARG_NO_CHECK;
	}
	
	if (argc > 0 && str_cmp(*argv, "--superblock") == 0) {
		--argc; ++argv;
		arg_flags |= ARG_SUPERBLOCK;
	}
	
	if (argc > 0 && str_cmp(*argv, "--block-groups") == 0) {
		--argc; ++argv;
		arg_flags |= ARG_BLOCK_GROUPS;
	}
	
	if (argc > 0 && str_cmp(*argv, "--inode") == 0) {
		--argc; ++argv;
		if (argc == 0) {
			printf(NAME ": Argument expected for --inode\n");
			return 2;
		}
		
		inode = strtol(*argv, &endptr, 10);
		if (*endptr != '\0') {
			printf(NAME ": Error, invalid argument for --inode.\n");
			syntax_print();
			return 1;
		}
		
		arg_flags |= ARG_INODE;
		--argc; ++argv;
		
		if (argc > 0 && str_cmp(*argv, "--blocks") == 0) {
			--argc; ++argv;
			arg_flags |= ARG_INODE_BLOCKS;
		}
		
		if (argc > 0 && str_cmp(*argv, "--data") == 0) {
			--argc; ++argv;
			if (argc == 0) {
				printf(NAME ": Argument expected for --data\n");
				return 2;
			}
			
			inode_data = strtol(*argv, &endptr, 10);
			if (*endptr != '\0') {
				printf(NAME ": Error, invalid argument for --data.\n");
				syntax_print();
				return 1;
			}
			
			arg_flags |= ARG_INODE_DATA;
			--argc; ++argv;
		}
		
		if (argc > 0 && str_cmp(*argv, "--list") == 0) {
			--argc; ++argv;
			arg_flags |= ARG_INODE_LIST;
		}
	}

	if (argc < 1) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}
	else if (argc > 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}
	assert(argc == 1);
	
	/* Display common things by default */
	if ((arg_flags & ARG_ALL) == 0) {
		arg_flags = ARG_COMMON;
	}

	dev_path = *argv;

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = ext2_filesystem_init(&filesystem, service_id);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libext2.\n");
		return 3;
	}
	
	rc = ext2_filesystem_check_sanity(&filesystem);
	if (rc != EOK) {
		printf(NAME ": Filesystem did not pass sanity check.\n");
		if (!(arg_flags & ARG_NO_CHECK)) {
			return 3;
		}
	}
	
	if (arg_flags & ARG_SUPERBLOCK) {
		print_superblock(filesystem.superblock);
	}
	
	if (arg_flags & ARG_BLOCK_GROUPS) {
		print_block_groups(&filesystem);
	}
	
	if (arg_flags & ARG_INODE) {
		print_inode_by_number(&filesystem, inode, arg_flags & ARG_INODE_DATA,
		    inode_data, arg_flags & ARG_INODE_LIST,
		    arg_flags & ARG_INODE_BLOCKS);
	}

	ext2_filesystem_fini(&filesystem);

	return 0;
}


static void syntax_print(void)
{
	printf("syntax: ext2info [--no-check] [--superblock] [--block-groups] "
	    "[--inode <i-number> [--blocks] [--data <block-number>] [--list]] "
	    "<device_name>\n");
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

void print_block_groups(ext2_filesystem_t *filesystem)
{
	uint32_t block_group_count;
	uint32_t i;
	ext2_block_group_ref_t *block_group_ref;
	int rc;
	
	printf("Block groups:\n");
	
	block_group_count = ext2_superblock_get_block_group_count(
	    filesystem->superblock);
	
	for (i = 0; i < block_group_count; i++) {
		printf("  Block group %u\n", i);
		rc = ext2_filesystem_get_block_group_ref(filesystem, i, &block_group_ref);
		if (rc != EOK) {
			printf("    Failed reading block group\n");
			continue;
		}
		
		print_block_group(block_group_ref->block_group);
		
		rc = ext2_filesystem_put_block_group_ref(block_group_ref);
		if (rc != EOK) {
			printf("    Failed freeing block group\n");
		}
	}
	
}

void print_block_group(ext2_block_group_t *bg)
{
	uint32_t block_bitmap_block;
	uint32_t inode_bitmap_block;
	uint32_t inode_table_first_block;
	uint16_t free_block_count;
	uint16_t free_inode_count;
	uint16_t directory_inode_count;
	
	block_bitmap_block = ext2_block_group_get_block_bitmap_block(bg);
	inode_bitmap_block = ext2_block_group_get_inode_bitmap_block(bg);
	inode_table_first_block = ext2_block_group_get_inode_table_first_block(bg);
	free_block_count = ext2_block_group_get_free_block_count(bg);
	free_inode_count = ext2_block_group_get_free_inode_count(bg);
	directory_inode_count = ext2_block_group_get_directory_inode_count(bg);
	
	printf("    Block bitmap block: %u\n", block_bitmap_block);
	printf("    Inode bitmap block: %u\n", inode_bitmap_block);
	printf("    Inode table's first block: %u\n", inode_table_first_block);
	printf("    Free blocks: %u\n", free_block_count);
	printf("    Free inodes: %u\n", free_inode_count);
	printf("    Directory inodes: %u\n", directory_inode_count);
}

void print_inode_by_number(ext2_filesystem_t *fs, uint32_t inode, 
    bool print_data, uint32_t data, bool list, bool blocks)
{
	int rc;
	ext2_inode_ref_t *inode_ref;
	
	printf("Inode %u\n", inode);
	
	rc = ext2_filesystem_get_inode_ref(fs, inode, &inode_ref);
	if (rc != EOK) {
		printf("  Failed getting inode ref\n");
		return;
	}
	
	print_inode(fs, inode_ref->inode, blocks);
	if (print_data) {
		print_inode_data(fs, inode_ref->inode, data);
	}
	
	if (list && ext2_inode_is_type(fs->superblock, inode_ref->inode,
	    EXT2_INODE_MODE_DIRECTORY)) {
		print_directory_contents(fs, inode_ref);
	}
	
	rc = ext2_filesystem_put_inode_ref(inode_ref);
	if (rc != EOK) {
		printf("  Failed putting inode ref\n");
	}
}

void print_inode(ext2_filesystem_t *fs, ext2_inode_t *inode, bool blocks)
{
	uint32_t mode;
	uint32_t mode_type;
	uint32_t user_id;
	uint32_t group_id;
	uint64_t size;
	uint16_t usage_count;
	uint32_t flags;
	uint16_t access;
	const char *type;
	uint32_t total_blocks;
	uint32_t i;
	uint32_t range_start;
	uint32_t range_last;
	uint32_t range_start_file;
	uint32_t range_last_file;
	uint32_t range_current;
	uint32_t range_expected;
	uint32_t block_size;
	uint64_t file_blocks;
	bool printed_range;
	int rc;
	
	block_size = ext2_superblock_get_block_size(fs->superblock);
	mode = ext2_inode_get_mode(fs->superblock, inode);
	mode_type = mode & EXT2_INODE_MODE_TYPE_MASK;
	user_id = ext2_inode_get_user_id(fs->superblock, inode);
	group_id = ext2_inode_get_group_id(fs->superblock, inode);
	size = ext2_inode_get_size(fs->superblock, inode);
	usage_count = ext2_inode_get_usage_count(inode);
	flags = ext2_inode_get_flags(inode);
	total_blocks = ext2_inode_get_reserved_blocks(fs->superblock, inode);
	file_blocks = 0;
	if (size > 0) {
		file_blocks = ((size-1)/block_size)+1;
	}
	
	type = "Unknown";
	if (mode_type == EXT2_INODE_MODE_BLOCKDEV) {
		type = "Block device";
	}
	else if (mode_type == EXT2_INODE_MODE_FIFO) {
		type = "Fifo (pipe)";
	}
	else if (mode_type == EXT2_INODE_MODE_CHARDEV) {
		type = "Character device";
	}
	else if (mode_type == EXT2_INODE_MODE_DIRECTORY) {
		type = "Directory";
	}
	else if (mode_type == EXT2_INODE_MODE_FILE) {
		type = "File";
	}
	else if (mode_type == EXT2_INODE_MODE_SOFTLINK) {
		type = "Soft link";
	}
	else if (mode_type == EXT2_INODE_MODE_SOCKET) {
		type = "Socket";
	}
	
	access = mode & EXT2_INODE_MODE_ACCESS_MASK;
	
	
	printf("  Mode: %08x (Type: %s, Access bits: %04ho)\n", mode, type, access);
	printf("  User ID: %u\n", user_id);
	printf("  Group ID: %u\n", group_id);
	printf("  Size: %" PRIu64 "\n", size);
	printf("  Usage (link) count: %u\n", usage_count);
	printf("  Flags: %u\n", flags);
	printf("  Total allocated blocks: %u\n", total_blocks);
	
	if (blocks) {
		printf("  Block list: ");
		
		range_start = 0;
		range_current = 0;
		range_last = 0;
		
		printed_range = false;
		
		for (i = 0; i <= file_blocks; i++) {
			if (i < file_blocks) {
				rc = ext2_filesystem_get_inode_data_block_index(fs, inode, i, &range_current);
				if (rc != EOK) {
					printf("Error reading data block indexes\n");
					return;
				}
			}
			if (range_last == 0) {
				range_expected = 0;
			}
			else {
				range_expected = range_last + 1;
			}
			if (range_current != range_expected) {
				if (i > 0) {
					if (printed_range) {
						printf(", ");
					}
					if (range_start == 0 && range_last == 0) {
						if (range_start_file == range_last_file) {
							printf("%u N/A", range_start_file);
						}
						else {
							printf("[%u, %u] N/A", range_start_file,
								range_last_file);
						}
					}
					else {
						if (range_start_file == range_last_file) {
							printf("%u -> %u", range_start_file, range_start);
						}
						else {
							printf("[%u, %u] -> [%u, %u]", range_start_file,
								range_last_file, range_start, range_last);
						}
					}
					printed_range = true;
				}
				range_start = range_current;
				range_start_file = i;
			}
			range_last = range_current;
			range_last_file = i;
		}
		printf("\n");
	}
}

void print_data(unsigned char *data, size_t size)
{
	unsigned char c;
	size_t i;
	
	for (i = 0; i < size; i++) {
		c = data[i];
		if (c >= 32 && c < 127) {
			putchar(c);
		}
		else {
			putchar('.');
		}
	}
}

void print_inode_data(ext2_filesystem_t *fs, ext2_inode_t *inode, uint32_t data)
{
	int rc;
	uint32_t data_block_index;
	block_t *block;
	
	rc = ext2_filesystem_get_inode_data_block_index(fs, inode, data,
	    &data_block_index);
	
	if (rc != EOK) {
		printf("Failed getting data block #%u\n", data);
		return;
	}
	
	printf("Data for inode contents block #%u is located in filesystem "
	    "block %u\n", data, data_block_index);
	
	printf("Data preview (only printable characters):\n");
	
	rc = block_get(&block, fs->device, data_block_index, 0);
	if (rc != EOK) {
		printf("Failed reading filesystem block %u\n", data_block_index);
		return;
	}
	
	print_data(block->data, block->size);
	printf("\n");	
	
	rc = block_put(block);
	if (rc != EOK) {
		printf("Failed putting filesystem block\n");
	}
	
}

void print_directory_contents(ext2_filesystem_t *fs,
    ext2_inode_ref_t *inode_ref)
{
	int rc;
	ext2_directory_iterator_t it;
	size_t name_size;
	uint32_t inode;
	
	printf("  Directory contents:\n");
	
	rc = ext2_directory_iterator_init(&it, fs, inode_ref, 0);
	if (rc != EOK) {
		printf("Failed initializing directory iterator\n");
		return;
	}
	
	while (it.current != NULL) {
		name_size = ext2_directory_entry_ll_get_name_length(fs->superblock,
		    it.current);
		printf("    ");
		print_data(&it.current->name, name_size);
		
		inode = ext2_directory_entry_ll_get_inode(it.current);
		printf(" --> %u\n", inode);
		
		rc = ext2_directory_iterator_next(&it);
		if (rc != EOK) {
			printf("Failed reading directory contents\n");
			goto cleanup;
		}
	}

cleanup:
	rc = ext2_directory_iterator_fini(&it);
	if (rc != EOK) {
		printf("Failed cleaning-up directory iterator\n");
	}
	
}

/**
 * @}
 */
