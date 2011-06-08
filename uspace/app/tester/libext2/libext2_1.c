/*
 * Copyright (c) 2011 Martin Sucha
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

#include <stdio.h>
#include <unistd.h>
#include "../tester.h"
#include "../util.h"
#include <libext2.h>
#include <malloc.h>

static ext2_superblock_t *fake_superblock1()
{
	uint8_t *buf;
	int i;
	
	buf = malloc(EXT2_SUPERBLOCK_SIZE);
	if (buf == NULL) {
		return NULL;
	}
	
	for (i = 0; i < EXT2_SUPERBLOCK_SIZE; i++) {
		buf[i] = i % 256;
	}
	
	return (ext2_superblock_t *) buf;
}

const char *test_libext2_1(void)
{
	ext2_superblock_t *fake1;
	
	TPRINTF("Testing libext2 superblock getters...\n");
	TPRINTF("Simple test for correct position and byte order\n");
	
	fake1 = fake_superblock1();
	if (fake1 == NULL) {
		return "Failed allocating memory for test superblock 1";
	}
	
	ASSERT_EQ_32(0x03020100, ext2_superblock_get_total_inode_count(fake1),
	    "Failed getting total inode count");
	ASSERT_EQ_32(0x07060504, ext2_superblock_get_total_block_count(fake1),
	    "Failed getting total block count");
	ASSERT_EQ_32(0x0B0A0908, ext2_superblock_get_reserved_block_count(fake1),
	    "Failed getting reserved block count");
	ASSERT_EQ_32(0x0F0E0D0C, ext2_superblock_get_free_block_count(fake1),
	    "Failed getting free block count");
	ASSERT_EQ_32(0x13121110, ext2_superblock_get_free_inode_count(fake1),
	    "Failed getting free inode count");
	ASSERT_EQ_32(0x17161514, ext2_superblock_get_first_block(fake1),
	    "Failed getting first block number");
	ASSERT_EQ_32(0x1B1A1918, ext2_superblock_get_block_size_log2(fake1),
	    "Failed getting log block size");
	ASSERT_EQ_32(0x1F1E1D1C, ext2_superblock_get_fragment_size_log2(fake1),
	    "Failed getting log fragment size");
	ASSERT_EQ_32(0x23222120, ext2_superblock_get_blocks_per_group(fake1),
	    "Failed getting blocks per group");
	ASSERT_EQ_32(0x27262524, ext2_superblock_get_fragments_per_group(fake1),
	    "Failed getting fragments per group");
	ASSERT_EQ_32(0x2B2A2928, ext2_superblock_get_inodes_per_group(fake1),
	    "Failed getting inodes per group");
	ASSERT_EQ_16(0x3938, ext2_superblock_get_magic(fake1),
	    "Failed getting magic number");
	ASSERT_EQ_16(0x3B3A, ext2_superblock_get_state(fake1),
	    "Failed getting state");
	ASSERT_EQ_16(0x3F3E, ext2_superblock_get_rev_minor(fake1),
	    "Failed getting minor revision number");
	ASSERT_EQ_32(0x4B4A4948, ext2_superblock_get_os(fake1),
	    "Failed getting OS");
	ASSERT_EQ_32(0x4F4E4D4C, ext2_superblock_get_rev_major(fake1),
	    "Failed getting major revision number");
	ASSERT_EQ_32(0x57565554, ext2_superblock_get_first_inode(fake1),
	    "Failed getting first inode number");
	ASSERT_EQ_16(0x5958, ext2_superblock_get_inode_size(fake1),
	    "Failed getting size");
	ASSERT_EQ_8(0x68, fake1->uuid[0], "UUID position is incorrect");
	ASSERT_EQ_8(0x78, fake1->volume_name[0],
	    "Volume name position is incorrect");
	
	return NULL;
}
