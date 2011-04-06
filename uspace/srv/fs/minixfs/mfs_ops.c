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
#include <assert.h>
#include <fibril_synch.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

static bool check_magic_number(uint16_t magic, bool *native,
				mfs_version_t *version, bool *longfilenames);
static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
			fs_index_t index);

static int mfs_node_put(fs_node_t *fsnode);
static int mfs_node_open(fs_node_t *fsnode);
static fs_index_t mfs_index_get(fs_node_t *fsnode);
static unsigned mfs_lnkcnt_get(fs_node_t *fsnode);
static char mfs_plb_get_char(unsigned pos);
static bool mfs_is_directory(fs_node_t *fsnode);
static bool mfs_is_file(fs_node_t *fsnode);
static int mfs_has_children(bool *has_children, fs_node_t *fsnode);
static int mfs_root_get(fs_node_t **rfn, devmap_handle_t handle);
static devmap_handle_t mfs_device_get(fs_node_t *fsnode);
static aoff64_t mfs_size_get(fs_node_t *node);
static int mfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component);
static int mfs_create_node(fs_node_t **rfn, devmap_handle_t handle, int flags);

static
int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index);


static LIST_INITIALIZE(inst_list);
static FIBRIL_MUTEX_INITIALIZE(inst_list_mutex);

libfs_ops_t mfs_libfs_ops = {
	.size_get = mfs_size_get,
	.root_get = mfs_root_get,
	.device_get = mfs_device_get,
	.is_directory = mfs_is_directory,
	.is_file = mfs_is_file,
	.node_get = mfs_node_get,
	.node_put = mfs_node_put,
	.node_open = mfs_node_open,
	.index_get = mfs_index_get,
	.match = mfs_match,
	.create = mfs_create_node,
	.plb_get_char = mfs_plb_get_char,
	.has_children = mfs_has_children,
	.lnkcnt_get = mfs_lnkcnt_get
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
	sbi = malloc(sizeof(*sbi));

	if (!sbi) {
		async_answer_0(rid, ENOMEM);
		return;
	}

	/*Allocate space for filesystem instance*/
	instance = malloc(sizeof(*instance));

	if (!instance) {
		async_answer_0(rid, ENOMEM);
		return;
	}

	sb = malloc(MFS_SUPERBLOCK_SIZE);

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
		/*This is a V1 or V2 Minix filesystem*/
		magic = sb->s_magic;
		goto recognized;
	}

	if (!check_magic_number(sb3->s_magic, &native, &version, &longnames)) {
		mfsdebug("magic number not recognized\n");
		block_fini(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	/*This is a V3 Minix filesystem*/

	magic = sb3->s_magic;

recognized:

	mfsdebug("magic number recognized = %04x\n", magic);

	/*Fill superblock info structure*/

	sbi->fs_version = version;
	sbi->long_names = longnames;
	sbi->native = native;
	sbi->magic = magic;
	sbi->isearch = 0;
	sbi->zsearch = 0;

	if (version == MFS_VERSION_V3) {
		sbi->ninodes = conv32(native, sb3->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb3->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb3->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb3->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb3->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb3->s_max_file_size);
		sbi->nzones = conv32(native, sb3->s_nzones);
		sbi->block_size = conv16(native, sb3->s_block_size);
		sbi->ino_per_block = V3_INODES_PER_BLOCK(sbi->block_size);
		sbi->dirsize = MFS3_DIRSIZE;
		sbi->max_name_len = MFS3_MAX_NAME_LEN;
	} else {
		sbi->ninodes = conv16(native, sb->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb->s_max_file_size);
		sbi->nzones = conv16(native, sb->s_nzones);
		sbi->block_size = MFS_BLOCKSIZE;
		sbi->ino_per_block = V1_INODES_PER_BLOCK;
		if (version == MFS_VERSION_V2)
			sbi->nzones = conv32(native, sb->s_nzones2);
		sbi->dirsize = longnames ? MFSL_DIRSIZE : MFS_DIRSIZE;
		sbi->max_name_len = longnames ? MFS_L_MAX_NAME_LEN :
				MFS_MAX_NAME_LEN;
	}
	sbi->itable_off = 2 + sbi->ibmap_blocks + sbi->zbmap_blocks;
 
	free(sb);

	rc = block_cache_init(devmap_handle, sbi->block_size, 0, cmode);

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

static int mfs_create_node(fs_node_t **rfn, devmap_handle_t handle, int flags)
{
	mfsdebug("create_node()\n");
	return ENOTSUP;
}

static int mfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	struct mfs_node *mnode = pfn->data;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_dentry_info *d_info;

	if (!S_ISDIR(ino_i->i_mode))
		return ENOTDIR;

	mfsdebug("mfs_match()\n");

	struct mfs_sb_info *sbi = mnode->instance->sbi;
	const size_t comp_size = str_size(component);

	int i = 2;
	while (1) {
		d_info = read_directory_entry(mnode, i++);
		if (!d_info) {
			/*Reached the end of the directory entry list*/
			break;
		}

		if (!d_info->d_inum) {
			/*This entry is not used*/
			free(d_info);
			continue;
		}

		if (!bcmp(component, d_info->d_name, min(sbi->max_name_len,
				comp_size))) {
			/*Hit!*/
			mfs_node_core_get(rfn, mnode->instance,
				d_info->d_inum);
			free(d_info);
			goto found;
		}
		free(d_info);
	}
	*rfn = NULL;
	return ENOENT;
found:
	return EOK;
}

static aoff64_t mfs_size_get(fs_node_t *node)
{
	assert(node);

	const struct mfs_node *mnode = node->data;
	assert(mnode);
	assert(mnode->ino_i);

	mfsdebug("inode size is %d\n", (int) mnode->ino_i->i_size);

	return mnode->ino_i->i_size;
}

void mfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	mfsdebug("mfs_stat()\n");
	libfs_stat(&mfs_libfs_ops, mfs_reg.fs_handle, rid, request);
}

static int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index)
{
	int rc;
	struct mfs_instance *instance;

	mfsdebug("mfs_node_get()\n");

	rc = mfs_instance_get(devmap_handle, &instance);

	if (rc != EOK)
		return rc;

	return mfs_node_core_get(rfn, instance, index);
}

static int mfs_node_put(fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("mfs_node_put()\n");

	put_inode(mnode);
	free(mnode->ino_i);
	free(mnode);

	return EOK;
}

static int mfs_node_open(fs_node_t *fsnode)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

static fs_index_t mfs_index_get(fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("mfs_index_get()\n");

	assert(mnode->ino_i);
	return mnode->ino_i->index;
}

static unsigned mfs_lnkcnt_get(fs_node_t *fsnode)
{
	unsigned rc;
	struct mfs_node *mnode = fsnode->data;

	assert(mnode);
	assert(mnode->ino_i);

	rc = mnode->ino_i->i_nlinks;
	mfsdebug("mfs_lnkcnt_get(): %u\n", rc);
	return rc;
}

static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
			fs_index_t index)
{
	fs_node_t *node = NULL;
	struct mfs_node *mnode = NULL;
	int rc;

	node = malloc(sizeof(fs_node_t));
	if (!node) {
		rc = ENOMEM;
		goto out_err;
	}

	fs_node_initialize(node);

	mnode = malloc(sizeof(*mnode));
	if (!mnode) {
		rc = ENOMEM;
		goto out_err;
	}

	struct mfs_ino_info *ino_i;

	rc = get_inode(inst, &ino_i, index);
	if (rc != EOK)
		goto out_err;

	ino_i->index = index;
	mnode->ino_i = ino_i;

	mnode->instance = inst;
	node->data = mnode;
	*rfn = node;

	mfsdebug("node_get_core(%d) OK\n", (int) index);

	return EOK;

out_err:
	if (node)
		free(node);
	if (mnode)
		free(mnode);
	return rc;
}

static bool mfs_is_directory(fs_node_t *fsnode)
{
	const struct mfs_node *node = fsnode->data;
	return S_ISDIR(node->ino_i->i_mode);
}

static bool mfs_is_file(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	return S_ISREG(node->ino_i->i_mode);
}

static int mfs_root_get(fs_node_t **rfn, devmap_handle_t handle)
{
	int rc = mfs_node_get(rfn, handle, MFS_ROOT_INO);

	mfsdebug("mfs_root_get %s\n", rc == EOK ? "OK" : "FAIL");
	return rc;
}

void mfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&mfs_libfs_ops, mfs_reg.fs_handle, rid, request);
}

static char mfs_plb_get_char(unsigned pos)
{
	return mfs_reg.plb_ro[pos % PLB_SIZE];
}

static int mfs_has_children(bool *has_children, fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;
	const struct mfs_ino_info *ino_i = mnode->ino_i;
	const struct mfs_instance *inst = mnode->instance;
	const struct mfs_sb_info *sbi = inst->sbi;
	uint32_t n_dentries = 0;

	*has_children = false;

	if (!S_ISDIR(mnode->ino_i->i_mode))
		goto out;

	struct mfs_dentry_info *d_info;
	n_dentries =  ino_i->i_size / sbi->dirsize;
	
	/* The first two dentries are always . and .. */
	assert(n_dentries >= 2);

	if (n_dentries == 2)
		goto out;

	int i = 2;
	while (1) {
		d_info = read_directory_entry(mnode, i++);

		if (!d_info) {
			/*Reached the end of the dentries list*/
			break;
		}

		if (d_info->d_inum) {
			/*A valid entry has been found*/
			*has_children = true;
			free(d_info);
			break;
		}

		free(d_info);
	}

out:

	if (n_dentries > 2 && !*has_children)
		printf(NAME ": Filesystem corruption detected\n");

	if (*has_children)
		mfsdebug("Has children\n");
	else
		mfsdebug("Has not children\n");

	return EOK;
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
		instance_ptr = list_get_instance(link, struct mfs_instance,
				link);
		
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
	bool rc = false;
	*longfilenames = false;

	if (magic == MFS_MAGIC_V1 || magic == MFS_MAGIC_V1R) {
		*native = magic == MFS_MAGIC_V1;
		*version = MFS_VERSION_V1;
		rc = true;
	} else if (magic == MFS_MAGIC_V1L || magic == MFS_MAGIC_V1LR) {
		*native = magic == MFS_MAGIC_V1L;
		*version = MFS_VERSION_V1;
		*longfilenames = true;
		rc = true;
	} else if (magic == MFS_MAGIC_V2 || magic == MFS_MAGIC_V2R) {
		*native = magic == MFS_MAGIC_V2;
		*version = MFS_VERSION_V2;
		rc = true;
	} else if (magic == MFS_MAGIC_V2L || magic == MFS_MAGIC_V2LR) {
		*native = magic == MFS_MAGIC_V2L;
		*version = MFS_VERSION_V2;
		*longfilenames = true;
		rc = true;
	} else if (magic == MFS_MAGIC_V3 || magic == MFS_MAGIC_V3R) {
		*native = magic == MFS_MAGIC_V3;
		*version = MFS_VERSION_V3;
		rc = true;
	}

	return rc;
}

/**
 * @}
 */ 

