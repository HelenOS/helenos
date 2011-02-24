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
#include "libext2_directory.h"
#include <byteorder.h>

/**
 * Get inode number for the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
inline uint32_t	ext2_directory_entry_ll_get_inode(ext2_directory_entry_ll_t *de)
{
	return uint32_t_le2host(de->inode);
}

/**
 * Get length of the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
inline uint16_t	ext2_directory_entry_ll_get_entry_length(
    ext2_directory_entry_ll_t *de)
{
	return uint16_t_le2host(de->entry_length);
}

/**
 * Get length of the name stored in the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
inline uint16_t	ext2_directory_entry_ll_get_name_length(
    ext2_superblock_t *sb, ext2_directory_entry_ll_t *de)
{
	if (ext2_superblock_get_rev_major(sb) == 0 &&
	    ext2_superblock_get_rev_minor(sb) < 5) {
		return ((uint16_t)de->name_length_high) << 8 | 
		    ((uint16_t)de->name_length);
	}
	return de->name_length;
}

/** @}
 */
