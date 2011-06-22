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

#ifndef LIBEXT2_LIBEXT2_FILESYSTEM_H_
#define LIBEXT2_LIBEXT2_FILESYSTEM_H_

#include <libblock.h>
#include "libext2_superblock.h"
#include "libext2_block_group.h"
#include "libext2_inode.h"

typedef struct ext2_filesystem {
	service_id_t		device;
	ext2_superblock_t *	superblock;
} ext2_filesystem_t;

// allow maximum this block size
#define EXT2_MAX_BLOCK_SIZE			8096
#define EXT2_REV0_FIRST_INODE		11
#define EXT2_REV0_INODE_SIZE		128

#define EXT2_FEATURE_RO_SPARSE_SUPERBLOCK	1
#define EXT2_FEATURE_RO_LARGE_FILE			2
#define EXT2_FEATURE_I_TYPE_IN_DIR			2

#define EXT2_SUPPORTED_INCOMPATIBLE_FEATURES EXT2_FEATURE_I_TYPE_IN_DIR
#define EXT2_SUPPORTED_READ_ONLY_FEATURES 0

extern int ext2_filesystem_init(ext2_filesystem_t *, service_id_t);
extern int ext2_filesystem_check_sanity(ext2_filesystem_t *);
extern int ext2_filesystem_check_flags(ext2_filesystem_t *, bool *);
extern int ext2_filesystem_get_block_group_ref(ext2_filesystem_t *, uint32_t, 
    ext2_block_group_ref_t **);
extern int ext2_filesystem_put_block_group_ref(ext2_block_group_ref_t *);
extern int ext2_filesystem_get_inode_ref(ext2_filesystem_t *, uint32_t,
    ext2_inode_ref_t **);
extern int ext2_filesystem_put_inode_ref(ext2_inode_ref_t *);
extern int ext2_filesystem_get_inode_data_block_index(ext2_filesystem_t *, ext2_inode_t*,
    aoff64_t, uint32_t*);
extern int ext2_filesystem_allocate_blocks(ext2_filesystem_t *, uint32_t *, size_t, uint32_t);
extern void ext2_filesystem_fini(ext2_filesystem_t *);

#endif

/** @}
 */
