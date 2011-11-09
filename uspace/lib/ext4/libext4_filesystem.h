/*
 * Copyright (c) 2011 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */ 

#ifndef LIBEXT4_LIBEXT4_FILESYSTEM_H_
#define LIBEXT4_LIBEXT4_FILESYSTEM_H_

#include <libblock.h>
#include "libext4_block_group.h"
#include "libext4_inode.h"
#include "libext4_superblock.h"

typedef struct ext4_filesystem {
	service_id_t device;
	ext4_superblock_t *	superblock;
	aoff64_t inode_block_limits[4];
	aoff64_t inode_blocks_per_level[4];
} ext4_filesystem_t;

#define EXT4_MAX_BLOCK_SIZE 	65536 //64 KiB
#define EXT4_REV0_INODE_SIZE	128


extern int ext4_filesystem_init(ext4_filesystem_t *, service_id_t);
extern void ext4_filesystem_fini(ext4_filesystem_t *fs);
extern int ext4_filesystem_check_sanity(ext4_filesystem_t *fs);
extern int ext4_filesystem_check_features(ext4_filesystem_t *, bool *);
extern int ext4_filesystem_get_block_group_ref(ext4_filesystem_t *, uint32_t,
    ext4_block_group_ref_t **);
extern int ext4_filesystem_put_block_group_ref(ext4_block_group_ref_t *);
extern int ext4_filesystem_get_inode_ref(ext4_filesystem_t *, uint32_t,
		ext4_inode_ref_t **);
extern int ext4_filesystem_put_inode_ref(ext4_inode_ref_t *);
extern int ext4_filesystem_get_inode_data_block_index(ext4_filesystem_t *,
	ext4_inode_t *, aoff64_t iblock, uint32_t *);
extern int ext4_filesystem_set_inode_data_block_index(ext4_filesystem_t *,
		ext4_inode_ref_t *, aoff64_t, uint32_t);
extern int ext4_filesystem_release_inode_block(ext4_filesystem_t *,
		ext4_inode_ref_t *, uint32_t);
#endif

/**
 * @}
 */
