/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup mfs
 * @{
 */

#ifndef _MFS_H_
#define _MFS_H_

#include <minix.h>
#include <macros.h>
#include <block.h>
#include <libfs.h>
#include <adt/list.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <macros.h>
#include "../../vfs/vfs.h"

#define NAME		"mfs"

/* #define DEBUG_MODE */

#ifdef DEBUG_MODE
#define mfsdebug(...)	printf(__VA_ARGS__)
#else
#define mfsdebug(...)
#endif

#define MFS_BMAP_START_BLOCK(sbi, bid) \
    ((bid) == BMAP_ZONE ? 2 + (sbi)->ibmap_blocks : 2)

#define MFS_BMAP_SIZE_BITS(sbi, bid) \
    ((bid) == BMAP_ZONE ? (sbi)->nzones - (sbi)->firstdatazone - 1 : \
    (sbi)->ninodes - 1)

#define MFS_BMAP_SIZE_BLOCKS(sbi, bid) \
    ((bid) == BMAP_ZONE ? (sbi)->zbmap_blocks : (sbi)->ibmap_blocks)

typedef uint32_t bitchunk_t;

typedef enum {
	BMAP_ZONE,
	BMAP_INODE
} bmap_id_t;

typedef enum {
	MFS_VERSION_V1 = 1,
	MFS_VERSION_V2,
	MFS_VERSION_V3
} mfs_version_t;

/* Generic MinixFS superblock */
struct mfs_sb_info {
	uint32_t ninodes;
	uint32_t nzones;
	unsigned long ibmap_blocks;
	unsigned long zbmap_blocks;
	unsigned long firstdatazone;
	int log2_zone_size;
	int block_size;
	uint32_t max_file_size;
	uint16_t magic;
	uint16_t state;

	/* The following fields do not exist on disk but only in memory */
	unsigned long itable_size;
	mfs_version_t fs_version;
	int ino_per_block;
	size_t dirsize;
	int itable_off;
	unsigned max_name_len;
	bool long_names;
	bool native;
	unsigned isearch;
	unsigned zsearch;

	/*
	 * Indicates wether if the cached number of free zones
	 * is to be considered valid or not.
	 */
	bool nfree_zones_valid;
	/*
	 * Cached number of free zones, used to avoid to scan
	 * the whole bitmap every time the mfs_free_block_count()
	 * is invoked.
	 */
	unsigned nfree_zones;
};

/* Generic MinixFS inode */
struct mfs_ino_info {
	uint16_t	i_mode;
	uint16_t	i_nlinks;
	int16_t		i_uid;
	uint16_t	i_gid;
	size_t		i_size;
	int32_t		i_atime;
	int32_t		i_mtime;
	int32_t		i_ctime;
	/* Block numbers for direct zones */
	uint32_t	i_dzone[V2_NR_DIRECT_ZONES];
	/* Block numbers for indirect zones */
	uint32_t	i_izone[V2_NR_INDIRECT_ZONES];

	/* The following fields do not exist on disk but only in memory */
	bool dirty;
	fs_index_t index;
};

/* Generic MFS directory entry */
struct mfs_dentry_info {
	uint32_t d_inum;
	char d_name[MFS3_MAX_NAME_LEN + 1];

	/* The following fields do not exist on disk but only in memory */

	/* Index of the dentry in the list */
	unsigned index;
	/* Pointer to the node at witch the dentry belongs */
	struct mfs_node *node;
};

struct mfs_instance {
	service_id_t service_id;
	struct mfs_sb_info *sbi;
	unsigned open_nodes_cnt;
};

/* MinixFS node in core */
struct mfs_node {
	struct mfs_ino_info *ino_i;
	struct mfs_instance *instance;
	unsigned refcnt;
	fs_node_t *fsnode;
	ht_link_t link;
};

/* mfs_ops.c */
extern vfs_out_ops_t mfs_ops;
extern libfs_ops_t mfs_libfs_ops;

extern errno_t
mfs_global_init(void);

/* mfs_inode.c */
extern errno_t
mfs_get_inode(struct mfs_instance *inst, struct mfs_ino_info **ino_i,
    fs_index_t index);

extern errno_t
mfs_put_inode(struct mfs_node *mnode);

extern errno_t
mfs_inode_shrink(struct mfs_node *mnode, size_t size_shrink);

/* mfs_rw.c */
extern errno_t
mfs_read_map(uint32_t *b, const struct mfs_node *mnode, const uint32_t pos);

extern errno_t
mfs_write_map(struct mfs_node *mnode, uint32_t pos, uint32_t new_zone,
    uint32_t *old_zone);

extern errno_t
mfs_prune_ind_zones(struct mfs_node *mnode, size_t new_size);

/* mfs_dentry.c */
extern errno_t
mfs_read_dentry(struct mfs_node *mnode,
    struct mfs_dentry_info *d_info, unsigned index);

extern errno_t
mfs_write_dentry(struct mfs_dentry_info *d_info);

extern errno_t
mfs_remove_dentry(struct mfs_node *mnode, const char *d_name);

extern errno_t
mfs_insert_dentry(struct mfs_node *mnode, const char *d_name, fs_index_t d_inum);

/* mfs_balloc.c */
extern errno_t
mfs_alloc_inode(struct mfs_instance *inst, uint32_t *inum);

extern errno_t
mfs_free_inode(struct mfs_instance *inst, uint32_t inum);

extern errno_t
mfs_alloc_zone(struct mfs_instance *inst, uint32_t *zone);

extern errno_t
mfs_free_zone(struct mfs_instance *inst, uint32_t zone);

extern errno_t
mfs_count_free_zones(struct mfs_instance *inst, uint32_t *zones);

extern errno_t
mfs_count_free_inodes(struct mfs_instance *inst, uint32_t *inodes);

/* mfs_utils.c */
extern uint16_t
conv16(bool native, uint16_t n);

extern uint32_t
conv32(bool native, uint32_t n);

extern uint64_t
conv64(bool native, uint64_t n);

#endif

/**
 * @}
 */
