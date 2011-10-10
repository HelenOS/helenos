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
} ext4_filesystem_t;

#define EXT4_MAX_BLOCK_SIZE 	65536 //64 KiB
#define EXT4_REV0_INODE_SIZE	128

/* Compatible features */
// TODO features comments !!!
#define EXT4_FEATURE_COMPAT_DIR_PREALLOC        0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES       0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL         0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR            0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE        0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX           0x0020

/* Read-only compatible features */
// TODO features comments !!!
#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER     0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE       0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR        0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE        0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM         0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK        0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE      0x0040

/* Incompatible features */
// TODO features comments !!!
#define EXT4_FEATURE_INCOMPAT_COMPRESSION       0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE          0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER           0x0004 /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV       0x0008 /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG           0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS           0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT             0x0080
#define EXT4_FEATURE_INCOMPAT_MMP               0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG           0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE          0x0400 /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA           0x1000 /* data in dirent */


// TODO MODIFY features corresponding with implementation
#define EXT4_FEATURE_COMPAT_SUPP EXT4_FEATURE_COMPAT_EXT_ATTR

#define EXT4_FEATURE_INCOMPAT_SUPP      (EXT4_FEATURE_INCOMPAT_FILETYPE| \
                                         EXT4_FEATURE_INCOMPAT_RECOVER| \
                                         EXT4_FEATURE_INCOMPAT_META_BG| \
                                         EXT4_FEATURE_INCOMPAT_EXTENTS| \
                                         EXT4_FEATURE_INCOMPAT_64BIT| \
                                         EXT4_FEATURE_INCOMPAT_FLEX_BG)

#define EXT4_FEATURE_RO_COMPAT_SUPP     (EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
                                         EXT4_FEATURE_RO_COMPAT_LARGE_FILE| \
                                         EXT4_FEATURE_RO_COMPAT_GDT_CSUM| \
                                         EXT4_FEATURE_RO_COMPAT_DIR_NLINK | \
                                         EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE | \
                                         EXT4_FEATURE_RO_COMPAT_BTREE_DIR |\
                                         EXT4_FEATURE_RO_COMPAT_HUGE_FILE)


extern int ext4_filesystem_init(ext4_filesystem_t *, service_id_t);
extern void ext4_filesystem_fini(ext4_filesystem_t *fs);
extern int ext4_filesystem_check_sanity(ext4_filesystem_t *fs);
extern int ext4_filesystem_check_features(ext4_filesystem_t *, bool *);
extern bool ext4_filesystem_has_feature_compatible(ext4_filesystem_t *, uint32_t);
extern bool ext4_filesystem_has_feature_incompatible(ext4_filesystem_t *, uint32_t);
extern bool ext4_filesystem_has_feature_read_only(ext4_filesystem_t *, uint32_t);
extern int ext4_filesystem_get_block_group_ref(ext4_filesystem_t *, uint32_t,
    ext4_block_group_ref_t **);
extern int ext4_filesystem_get_inode_ref(ext4_filesystem_t *, uint32_t,
		ext4_inode_ref_t **);
extern int ext4_filesystem_put_inode_ref(ext4_inode_ref_t *);
extern int ext4_filesystem_get_inode_data_block_index(ext4_filesystem_t *,
	ext4_inode_t *, aoff64_t iblock, uint32_t *);

#endif

/**
 * @}
 */
