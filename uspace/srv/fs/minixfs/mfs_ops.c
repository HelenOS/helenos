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

#include <stdio.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

static bool check_magic_number(uint16_t magic, bool *native,
				mfs_version_t *version, bool *longfilenames);
static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
			fs_index_t index);

static LIST_INITIALIZE(inst_list);
static FIBRIL_MUTEX_INITIALIZE(inst_list_mutex);

libfs_ops_t mfs_libfs_ops = {
	.device_get = mfs_device_get,
	.is_directory = mfs_is_directory,
	.is_file = mfs_is_file,
	.node_get = mfs_node_get
};

void mfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	enum cache_mode cmode;	
	struct mfs_superblock *sb;
	struct mfs3_superblock *sb3;
	struct mfs_sb_info *sbi;
	struct mfs_instance *instance;
	bool native, longnames;
	mfs_version_t version;
	uint16_t magic;

	/* Accept the mount options */
	char *opts;
	int rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);
	
	if (rc != EOK) {
		mfsdebug("Can't accept async data write\n");
		async_answer_0(rid, rc);
		return;
	}

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	free(opts);

	/* initialize libblock */
	rc = block_init(devmap_handle, 1024);
	if (rc != EOK) {
		mfsdebug("libblock initialization failed\n");
		async_answer_0(rid, rc);
		return;
	}

	/*Allocate space for generic MFS superblock*/
	sbi = (struct mfs_sb_info *) malloc(sizeof(struct mfs_sb_info));

	if (!sbi) {
		async_answer_0(rid, ENOMEM);
		return;
	}

	/*Allocate space for filesystem instance*/
	instance = (struct mfs_instance *) malloc(sizeof(struct mfs_instance));

	if (!instance) {
		async_answer_0(rid, ENOMEM);
		return;
	}

	sb = (struct mfs_superblock *) malloc(MFS_SUPERBLOCK_SIZE);

	if (!sb) {
		async_answer_0(rid, ENOMEM);
		return;
	}

	/* Read the superblock */
	rc = block_read_direct(devmap_handle, MFS_SUPERBLOCK << 1, 1, sb);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	sb3 = (struct mfs3_superblock *) sb;

	if (check_magic_number(sb->s_magic, &native, &version, &longnames)) {
		magic = sb->s_magic;
		goto recognized;
	}

	if (!check_magic_number(sb3->s_magic, &native, &version, &longnames)) {
		mfsdebug("magic number not recognized\n");
		block_fini(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	magic = sb3->s_magic;

recognized:

	mfsdebug("magic number recognized = %04x\n", magic);

	/*Fill superblock info structure*/

	sbi->fs_version = version;
	sbi->long_names = longnames;
	sbi->native = native;
	sbi->magic = magic;

	if (version == MFS_VERSION_V3) {
		sbi->ninodes = conv32(native, sb3->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb3->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb3->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb3->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb3->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb3->s_max_file_size);
		sbi->nzones = conv32(native, sb3->s_nzones);
		sbi->block_size = conv16(native, sb3->s_block_size);
	} else {
		sbi->ninodes = conv16(native, sb->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb->s_max_file_size);
		sbi->nzones = conv16(native, sb->s_nzones);
		sbi->block_size = MFS_BLOCKSIZE;
		if (version == MFS_VERSION_V2)
			sbi->nzones = conv32(native, sb->s_nzones2);
	}
 
	free(sb);

	rc = block_cache_init(devmap_handle, sbi->block_size, 0, CACHE_MODE_WT);

	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, EINVAL);
		mfsdebug("block cache initialization failed\n");
		return;
	}

	/*Initialize the instance structure and add it to the list*/
	link_initialize(&instance->link);
	instance->handle = devmap_handle;
	instance->sbi = sbi;

	fibril_mutex_lock(&inst_list_mutex);
	list_append(&instance->link, &inst_list);
	fibril_mutex_unlock(&inst_list_mutex);

	mfsdebug("mount successful\n");

	async_answer_0(rid, EOK);
}

void mfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&mfs_libfs_ops, mfs_reg.fs_handle, rid, request);
}

devmap_handle_t mfs_device_get(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	return node->instance->handle;
}

int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index)
{
	int rc;
	struct mfs_instance *instance;

	rc = mfs_instance_get(devmap_handle, &instance);

	if (rc != EOK)
		return rc;

	return mfs_node_core_get(rfn, instance, index);
}

static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
			fs_index_t index)
{
	fs_node_t *node = NULL;
	struct mfs_node *mnode = NULL;
	int rc;

	const struct mfs_sb_info *sbi = inst->sbi;

	node = (fs_node_t *) malloc(sizeof(fs_node_t));
	if (!node) {
		rc = ENOMEM;
		goto out_err;
	}

	fs_node_initialize(node);

	mnode = (struct mfs_node *) malloc(sizeof(struct mfs_node));
	if (!mnode) {
		rc = ENOMEM;
		goto out_err;
	}

	if (sbi->fs_version == MFS_VERSION_V1) {
		/*Read MFS V1 inode*/
		struct mfs_inode *ino;

		ino = mfs_read_inode_raw(inst, index);
		mnode->ino = ino;
	} else {
		/*Read MFS V2/V3 inode*/
		struct mfs2_inode *ino2;

		ino2 = mfs2_read_inode_raw(inst, index);
		mnode->ino2 = ino2;
	}

	mnode->instance = inst;
	node->data = mnode;
	*rfn = node;

	return EOK;

out_err:
	if (node)
		free(node);
	if (mnode)
		free(mnode);
	return rc;
}

bool mfs_is_directory(fs_node_t *fsnode)
{
	const struct mfs_node *node = fsnode->data;
	const struct mfs_sb_info *sbi = node->instance->sbi;

	if (sbi->fs_version == MFS_VERSION_V1)
		return S_ISDIR(node->ino->i_mode);
	else
		return S_ISDIR(node->ino2->i_mode);
}

bool mfs_is_file(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	struct mfs_sb_info *sbi = node->instance->sbi;

	if (sbi->fs_version == MFS_VERSION_V1)
		return S_ISREG(node->ino->i_mode);
	else
		return S_ISREG(node->ino2->i_mode);
}

/*
 * Find a filesystem instance given the devmap handle
 */
int mfs_instance_get(devmap_handle_t handle, struct mfs_instance **instance)
{
	link_t *link;
	struct mfs_instance *instance_ptr;

	fibril_mutex_lock(&inst_list_mutex);

	for (link = inst_list.next; link != &inst_list; link = link->next) {
		instance_ptr = list_get_instance(link, struct mfs_instance, link);
		
		if (instance_ptr->handle == handle) {
			*instance = instance_ptr;
			fibril_mutex_unlock(&inst_list_mutex);
			return EOK;
		}
	}

	mfsdebug("Instance not found\n");

	fibril_mutex_unlock(&inst_list_mutex);
	return EINVAL;
}

static bool check_magic_number(uint16_t magic, bool *native,
				mfs_version_t *version, bool *longfilenames)
{
	*longfilenames = false;

	if (magic == MFS_MAGIC_V1 || magic == MFS_MAGIC_V1R) {
		*native = magic == MFS_MAGIC_V1;
		*version = MFS_VERSION_V1;
		return true;
	} else if (magic == MFS_MAGIC_V1L || magic == MFS_MAGIC_V1LR) {
		*native = magic == MFS_MAGIC_V1L;
		*version = MFS_VERSION_V1;
		*longfilenames = true;
		return true;
	} else if (magic == MFS_MAGIC_V2 || magic == MFS_MAGIC_V2R) {
		*native = magic == MFS_MAGIC_V2;
		*version = MFS_VERSION_V2;
		return true;
	} else if (magic == MFS_MAGIC_V2L || magic == MFS_MAGIC_V2LR) {
		*native = magic == MFS_MAGIC_V2L;
		*version = MFS_VERSION_V2;
		*longfilenames = true;
		return true;
	} else if (magic == MFS_MAGIC_V3 || magic == MFS_MAGIC_V3R) {
		*native = magic == MFS_MAGIC_V3;
		*version = MFS_VERSION_V3;
		return true;
	}

	return false;
}

/**
 * @}
 */ 

