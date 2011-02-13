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

/** @addtogroup libext2
 * @{
 */
/**
 * @file
 */

#include "libext2.h"

/**
 * Return a magic number from ext2 superblock, this should be equal to
 * EXT_SUPERBLOCK_MAGIC for valid ext2 superblock
 */
inline uint16_t ext2_superblock_get_magic(ext2_superblock_t *sb) {
	return uint16_t_le2host(sb->magic);
}

/**
 * Get the position of first ext2 data block (i.e. the block number
 * containing main superblock)
 */
inline uint32_t ext2_superblock_get_first_block(ext2_superblock_t *sb) {
	return uint32_t_le2host(sb->first_block);
}

/**
 * Get the number of bits to shift a value of 1024 to the left necessary
 * to get the size of a block
 */
inline uint32_t ext2_superblock_get_block_size_log2(ext2_superblock_t *sb) {
	return uint32_t_le2host(sb->block_size_log2);
}

/**
 * Get the size of a block, in bytes 
 */
inline uint32_t ext2_superblock_get_block_size(ext2_superblock_t *sb) {
	return 1024 << ext2_superblock_get_block_size(sb);
}

/**
 * Get the number of bits to shift a value of 1024 to the left necessary
 * to get the size of a fragment (note that this is a signed integer and
 * if negative, the value should be shifted to the right instead)
 */
inline int32_t ext2_superblock_get_fragment_size_log2(ext2_superblock_t *sb) {
	return uint32_t_le2host(sb->fragment_size_log2);
}

/**
 * Get the size of a fragment, in bytes 
 */
inline uint32_t ext2_superblock_get_fragment_size(ext2_superblock_t *sb) {
	int32_t log = ext2_superblock_get_fragment_size_log2(sb);
	if (log >= 0) {
		return 1024 << log;
	}
	else {
		return 1024 >> -log;
	}
}

/**
 * Get number of blocks per block group
 */
inline uint32_t ext2_superblock_get_blocks_per_group(ext2_superblock_t *sb) {
	return uint32_t_le2host(sb->blocks_per_group);
}

/**
 * Get number of fragments per block group
 */
inline uint32_t ext2_superblock_get_fragments_per_group(ext2_superblock_t *sb) {
	return uint32_t_le2host(sb->fragments_per_group);
}


/** @}
 */
