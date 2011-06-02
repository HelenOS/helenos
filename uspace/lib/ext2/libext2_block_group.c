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
#include "libext2_block_group.h"
#include <byteorder.h>

/**
 * Get block ID corresponding to the block bitmap of this block group
 * 
 * @param bg pointer to block group descriptor
 */
uint32_t ext2_block_group_get_block_bitmap_block(ext2_block_group_t *bg)
{
	return uint32_t_le2host(bg->block_bitmap_block);
}

/**
 * Get block ID corresponding to the inode bitmap of this block group
 * 
 * @param bg pointer to block group descriptor
 */
uint32_t ext2_block_group_get_inode_bitmap_block(ext2_block_group_t *bg)
{
	return uint32_t_le2host(bg->inode_bitmap_block);
}

/**
 * Get block ID of first block in inode table
 * 
 * @param bg pointer to block group descriptor
 */
uint32_t ext2_block_group_get_inode_table_first_block(ext2_block_group_t *bg)
{
	return uint32_t_le2host(bg->inode_table_first_block);
}

/**
 * Get amount of free blocks in this block group
 * 
 * @param bg pointer to block group descriptor
 */
uint16_t ext2_block_group_get_free_block_count(ext2_block_group_t *bg)
{
	return uint16_t_le2host(bg->free_block_count);
}

/**
 * Set amount of free blocks in this block group
 * 
 * @param bg pointer to block group descriptor
 * @param val new value
 */
void ext2_block_group_set_free_block_count(ext2_block_group_t *bg,
	uint16_t val)
{
	bg->free_block_count = host2uint16_t_le(val);
}

/**
 * Get amount of free inodes in this block group
 * 
 * @param bg pointer to block group descriptor
 */
uint16_t ext2_block_group_get_free_inode_count(ext2_block_group_t *bg)
{
	return uint16_t_le2host(bg->free_inode_count);
}

/**
 * Get amount of inodes allocated for directories
 * 
 * @param bg pointer to block group descriptor
 */
uint16_t ext2_block_group_get_directory_inode_count(ext2_block_group_t *bg)
{
	return uint16_t_le2host(bg->directory_inode_count);
}


/** @}
 */
