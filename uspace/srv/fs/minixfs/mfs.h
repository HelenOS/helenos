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

/** @addtogroup fs
 * @{
 */ 

#ifndef _MFS_H_
#define _MFS_H_

#include <minix.h>
#include <libblock.h>
#include <libfs.h>
#include <adt/list.h>
#include "../../vfs/vfs.h"

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define mfsdebug(...)	printf(__VA_ARGS__)
#else
#define mfsdebug(...)
#endif

#ifdef _MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL fs_reg_t mfs_reg;

typedef enum {
	MFS_VERSION_V1 = 1,
	MFS_VERSION_V2,
	MFS_VERSION_V3
} mfs_version_t;

/*Generic MinixFS superblock*/
struct mfs_sb_info {
	uint32_t ninodes;
	uint32_t nzones;
	unsigned long ibmap_blocks;
	unsigned long zbmap_blocks;
	unsigned long firstdatazone;
	unsigned long itable_size;
	int log2_zone_size;
	int ino_per_block;
	int dirsize;
	int block_size;
	mfs_version_t fs_version;
	uint32_t max_file_size;
	uint16_t magic;
	uint16_t state;
	bool long_names;
	bool native;
};

/*Generic MinixFS inode*/
struct mfs_ino_info {
        uint16_t        i_mode;
        uint16_t        i_nlinks;
        int16_t         i_uid;
        uint16_t        i_gid;
        int32_t         i_size;
        int32_t         i_atime;
        int32_t         i_mtime;
        int32_t         i_ctime;
        /*Block numbers for direct zones*/
        uint32_t        i_dzone[V2_NR_DIRECT_ZONES];
        /*Block numbers for indirect zones*/
        uint32_t        i_izone[V2_NR_INDIRECT_ZONES];
};

struct mfs_instance {
	link_t link;
	devmap_handle_t handle;
	struct mfs_sb_info *sbi;
};

/*MinixFS node in core*/
struct mfs_node {
	struct mfs_ino_info *ino_i;
	struct mfs_instance *instance;
};

/*mfs_ops.c*/
extern void mfs_mounted(ipc_callid_t rid, ipc_call_t *request);
extern void mfs_mount(ipc_callid_t rid, ipc_call_t *request);
extern aoff64_t mfs_size_get(fs_node_t *node);
extern bool mfs_is_directory(fs_node_t *fsnode);
extern bool mfs_is_file(fs_node_t *fsnode);
extern int mfs_root_get(fs_node_t **rfn, devmap_handle_t handle);
extern devmap_handle_t mfs_device_get(fs_node_t *fsnode);
extern int mfs_instance_get(devmap_handle_t handle,
				struct mfs_instance **instance);
int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index);

extern void mfs_stat(ipc_callid_t rid, ipc_call_t *request);

/*mfs_inode.c*/
extern
struct mfs_ino_info *mfs_read_inode_raw(const struct mfs_instance *instance, 
					uint16_t inum);
extern
struct mfs_ino_info *mfs2_read_inode_raw(const struct mfs_instance *instance,
					uint32_t inum);

/*mfs_read.c*/
int read_map(uint32_t *b, const struct mfs_node *mnode, const uint32_t pos);

#endif

/**
 * @}
 */ 

