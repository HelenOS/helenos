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

#include <stdlib.h>
#include <fibril_synch.h>
#include <align.h>
#include <adt/hash_table.h>
#include "mfs.h"

#define OPEN_NODES_KEYS 2
#define OPEN_NODES_DEV_HANDLE_KEY 0
#define OPEN_NODES_INODE_KEY 1
#define OPEN_NODES_BUCKETS 256

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
static int mfs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name);
static int mfs_unlink(fs_node_t *, fs_node_t *, const char *name);
static int mfs_destroy_node(fs_node_t *fn);
static hash_index_t open_nodes_hash(unsigned long key[]);
static int open_nodes_compare(unsigned long key[], hash_count_t keys,
		link_t *item);
static void open_nodes_remove_cb(link_t *link);

static int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index);


static LIST_INITIALIZE(inst_list);
static FIBRIL_MUTEX_INITIALIZE(inst_list_mutex);
static hash_table_t open_nodes;
static FIBRIL_MUTEX_INITIALIZE(open_nodes_lock);

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
	.link = mfs_link,
	.unlink = mfs_unlink,
	.destroy = mfs_destroy_node,
	.plb_get_char = mfs_plb_get_char,
	.has_children = mfs_has_children,
	.lnkcnt_get = mfs_lnkcnt_get
};

/* Hash table interface for open nodes hash table */
static hash_index_t open_nodes_hash(unsigned long key[])
{
	/* TODO: This is very simple and probably can be improved */
	return key[OPEN_NODES_INODE_KEY] % OPEN_NODES_BUCKETS;
}

static int open_nodes_compare(unsigned long key[], hash_count_t keys,
		link_t *item)
{
	struct mfs_node *mnode = hash_table_get_instance(item, struct mfs_node, link);
	assert(keys > 0);
	if (mnode->instance->handle !=
	    ((devmap_handle_t) key[OPEN_NODES_DEV_HANDLE_KEY])) {
		return false;
	}
	if (keys == 1) {
		return true;
	}
	assert(keys == 2);
	return (mnode->ino_i->index == key[OPEN_NODES_INODE_KEY]);
}

static void open_nodes_remove_cb(link_t *link)
{
	/* We don't use remove callback for this hash table */
}

static hash_table_operations_t open_nodes_ops = {
	.hash = open_nodes_hash,
	.compare = open_nodes_compare,
	.remove_callback = open_nodes_remove_cb,
};

int mfs_global_init(void)
{
	if (!hash_table_create(&open_nodes, OPEN_NODES_BUCKETS,
			OPEN_NODES_KEYS, &open_nodes_ops)) {
		return ENOMEM;
	}
	return EOK;
}

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
	rc = block_init(EXCHANGE_SERIALIZE, devmap_handle, 1024);
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

	instance->open_nodes_cnt = 0;

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
		sbi->block_size = MFS_BLOCKSIZE;
		if (version == MFS_VERSION_V2) {
			sbi->nzones = conv32(native, sb->s_nzones2);
			sbi->ino_per_block = V2_INODES_PER_BLOCK;
		} else {
			sbi->nzones = conv16(native, sb->s_nzones);
			sbi->ino_per_block = V1_INODES_PER_BLOCK;
		}
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

void mfs_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&mfs_libfs_ops, rid, request);
}

void mfs_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap = (devmap_handle_t) IPC_GET_ARG1(*request);
	struct mfs_instance *inst;

	int r = mfs_instance_get(devmap, &inst);
	if (r != EOK) {
		async_answer_0(rid, r);
		return;
	}

	if (inst->open_nodes_cnt != 0) {
		async_answer_0(rid, EBUSY);
		return;
	}

	(void) block_cache_fini(devmap);
	block_fini(devmap);

	/* Remove the instance from the list */
	fibril_mutex_lock(&inst_list_mutex);
	list_remove(&inst->link);
	fibril_mutex_unlock(&inst_list_mutex);

	free(inst->sbi);
	free(inst);

	async_answer_0(rid, EOK);
}

devmap_handle_t mfs_device_get(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	return node->instance->handle;
}

static int mfs_create_node(fs_node_t **rfn, devmap_handle_t handle, int flags)
{
	int r;
	struct mfs_instance *inst;
	struct mfs_node *mnode;
	fs_node_t *fsnode;
	uint32_t inum;

	mfsdebug("%s()\n", __FUNCTION__);

	r = mfs_instance_get(handle, &inst);
	on_error(r, return r);

	/*Alloc a new inode*/
	r = mfs_alloc_inode(inst, &inum);
	on_error(r, return r);

	struct mfs_ino_info *ino_i;

	ino_i = malloc(sizeof(*ino_i));
	if (!ino_i) {
		r = ENOMEM;
		goto out_err;
	}

	mnode = malloc(sizeof(*mnode));
	if (!mnode) {
		r = ENOMEM;
		goto out_err_1;
	}

	fsnode = malloc(sizeof(fs_node_t));
	if (!fsnode) {
		r = ENOMEM;
		goto out_err_2;
	}

	if (flags & L_DIRECTORY)
		ino_i->i_mode = S_IFDIR;
	else
		ino_i->i_mode = S_IFREG;

	ino_i->i_nlinks = 1;
	ino_i->i_uid = 0;
	ino_i->i_gid = 0;
	ino_i->i_size = 0;
	ino_i->i_atime = 0;
	ino_i->i_mtime = 0;
	ino_i->i_ctime = 0;

	memset(ino_i->i_dzone, 0, sizeof(uint32_t) * V2_NR_DIRECT_ZONES);
	memset(ino_i->i_izone, 0, sizeof(uint32_t) * V2_NR_INDIRECT_ZONES);

	mfsdebug("new node idx = %d\n", (int) inum);

	ino_i->index = inum;
	ino_i->dirty = true;
	mnode->ino_i = ino_i;
	mnode->instance = inst;
	mnode->refcnt = 1;

	link_initialize(&mnode->link);

	unsigned long key[] = {
		[OPEN_NODES_DEV_HANDLE_KEY] = inst->handle,
		[OPEN_NODES_INODE_KEY] = inum,
	};

	fibril_mutex_lock(&open_nodes_lock);
	hash_table_insert(&open_nodes, key, &mnode->link);
	fibril_mutex_unlock(&open_nodes_lock);
	inst->open_nodes_cnt++;

	mnode->ino_i->dirty = true;

	fs_node_initialize(fsnode);
	fsnode->data = mnode;
	mnode->fsnode = fsnode;
	*rfn = fsnode;

	return EOK;

out_err_2:
	free(mnode);
out_err_1:
	free(ino_i);
out_err:
	return r;
}

static int mfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	struct mfs_node *mnode = pfn->data;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_dentry_info d_info;
	int r;

	mfsdebug("%s()\n", __FUNCTION__);

	if (!S_ISDIR(ino_i->i_mode))
		return ENOTDIR;

	struct mfs_sb_info *sbi = mnode->instance->sbi;
	const size_t comp_size = str_size(component);

	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = read_dentry(mnode, &d_info, i);
		on_error(r, return r);

		if (!d_info.d_inum) {
			/*This entry is not used*/
			continue;
		}

		const size_t dentry_name_size = str_size(d_info.d_name);

		if (comp_size == dentry_name_size &&
			!bcmp(component, d_info.d_name, dentry_name_size)) {
			/*Hit!*/
			mfs_node_core_get(rfn, mnode->instance,
					  d_info.d_inum);
			goto found;
		}
	}
	*rfn = NULL;
found:
	return EOK;
}

static aoff64_t mfs_size_get(fs_node_t *node)
{
	assert(node);

	const struct mfs_node *mnode = node->data;
	assert(mnode);
	assert(mnode->ino_i);

	return mnode->ino_i->i_size;
}

void mfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&mfs_libfs_ops, mfs_reg.fs_handle, rid, request);
}

static int mfs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle,
			fs_index_t index)
{
	int rc;
	struct mfs_instance *instance;

	mfsdebug("%s()\n", __FUNCTION__);

	rc = mfs_instance_get(devmap_handle, &instance);
	on_error(rc, return rc);

	return mfs_node_core_get(rfn, instance, index);
}

static int mfs_node_put(fs_node_t *fsnode)
{
	int rc = EOK;
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("%s()\n", __FUNCTION__);

	fibril_mutex_lock(&open_nodes_lock);

	assert(mnode->refcnt > 0);
	mnode->refcnt--;
	if (mnode->refcnt == 0) {
		unsigned long key[] = {
			[OPEN_NODES_DEV_HANDLE_KEY] = mnode->instance->handle,
			[OPEN_NODES_INODE_KEY] = mnode->ino_i->index
		};
		hash_table_remove(&open_nodes, key, OPEN_NODES_KEYS);
		assert(mnode->instance->open_nodes_cnt > 0);
		mnode->instance->open_nodes_cnt--;
		rc = put_inode(mnode);
		free(mnode->ino_i);
		free(mnode);
		free(fsnode);
	}

	fibril_mutex_unlock(&open_nodes_lock);
	return rc;
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

	assert(mnode->ino_i);
	return mnode->ino_i->index;
}

static unsigned mfs_lnkcnt_get(fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("%s()\n", __FUNCTION__);

	assert(mnode);
	assert(mnode->ino_i);

	return mnode->ino_i->i_nlinks;
}

static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
			     fs_index_t index)
{
	fs_node_t *node = NULL;
	struct mfs_node *mnode = NULL;
	int rc;

	mfsdebug("%s()\n", __FUNCTION__);

	fibril_mutex_lock(&open_nodes_lock);

	/* Check if the node is not already open */
	unsigned long key[] = {
		[OPEN_NODES_DEV_HANDLE_KEY] = inst->handle,
		[OPEN_NODES_INODE_KEY] = index,
	};
	link_t *already_open = hash_table_find(&open_nodes, key);

	if (already_open) {
		mnode = hash_table_get_instance(already_open, struct mfs_node, link);
		*rfn = mnode->fsnode;
		mnode->refcnt++;

		fibril_mutex_unlock(&open_nodes_lock);
		return EOK;
	}

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
	on_error(rc, goto out_err);

	ino_i->index = index;
	mnode->ino_i = ino_i;
	mnode->refcnt = 1;
	link_initialize(&mnode->link);

	mnode->instance = inst;
	node->data = mnode;
	mnode->fsnode = node;
	*rfn = node;

	hash_table_insert(&open_nodes, key, &mnode->link);
	inst->open_nodes_cnt++;

	fibril_mutex_unlock(&open_nodes_lock);

	return EOK;

out_err:
	if (node)
		free(node);
	if (mnode)
		free(mnode);
	fibril_mutex_unlock(&open_nodes_lock);
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

static int mfs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	struct mfs_node *parent = pfn->data;
	struct mfs_node *child = cfn->data;
	struct mfs_sb_info *sbi = parent->instance->sbi;

	mfsdebug("%s()\n", __FUNCTION__);

	if (str_size(name) > sbi->max_name_len)
		return ENAMETOOLONG;

	int r = insert_dentry(parent, name, child->ino_i->index);
	on_error(r, goto exit_error);

	if (S_ISDIR(child->ino_i->i_mode)) {
		r = insert_dentry(child, ".", child->ino_i->index);
		on_error(r, goto exit_error);
		child->ino_i->i_nlinks++;
		child->ino_i->dirty = true;
		r = insert_dentry(child, "..", parent->ino_i->index);
		on_error(r, goto exit_error);
		parent->ino_i->i_nlinks++;
		parent->ino_i->dirty = true;
	}

exit_error:
	return r;
}

static int
mfs_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	struct mfs_node *parent = pfn->data;
	struct mfs_node *child = cfn->data;
	bool has_children;
	int r;

	mfsdebug("%s()\n", __FUNCTION__);

	if (!parent)
		return EBUSY;

	r = mfs_has_children(&has_children, cfn);
	on_error(r, return r);

	if (has_children)
		return ENOTEMPTY;

	r = remove_dentry(parent, name);
	on_error(r, return r);

	struct mfs_ino_info *chino = child->ino_i;

	assert(chino->i_nlinks >= 1);
	--chino->i_nlinks;

	if (chino->i_nlinks == 0 && S_ISDIR(chino->i_mode)) {
		parent->ino_i->i_nlinks--;
		parent->ino_i->dirty = true;
	}

	chino->dirty = true;

	return r;
}

static int mfs_has_children(bool *has_children, fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	int r;

	*has_children = false;

	if (!S_ISDIR(mnode->ino_i->i_mode))
		goto out;

	struct mfs_dentry_info d_info;

	/* The first two dentries are always . and .. */
	unsigned i;
	for (i = 2; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = read_dentry(mnode, &d_info, i);
		on_error(r, return r);

		if (d_info.d_inum) {
			/*A valid entry has been found*/
			*has_children = true;
			break;
		}
	}
out:

	return EOK;
}

void
mfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	int rc;
	devmap_handle_t handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	aoff64_t pos = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request),
					       IPC_GET_ARG4(*request));
	fs_node_t *fn;

	rc = mfs_node_get(&fn, handle, index);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		async_answer_0(rid, ENOENT);
		return;
	}

	struct mfs_node *mnode;
	struct mfs_ino_info *ino_i;
	size_t len, bytes = 0;
	ipc_callid_t callid;

	mnode = fn->data;
	ino_i = mnode->ino_i;

	if (!async_data_read_receive(&callid, &len)) {
		rc = EINVAL;
		goto out_error;
	}

	if (S_ISDIR(ino_i->i_mode)) {
		aoff64_t spos = pos;
		struct mfs_dentry_info d_info;
		struct mfs_sb_info *sbi = mnode->instance->sbi;

		if (pos < 2) {
			/*Skip the first two dentries ('.' and '..')*/
			pos = 2;
		}

		for (; pos < mnode->ino_i->i_size / sbi->dirsize; ++pos) {
			rc = read_dentry(mnode, &d_info, pos);
			on_error(rc, goto out_error);

			if (d_info.d_inum) {
				/*Dentry found!*/
				goto found;
			}
		}

		rc = mfs_node_put(fn);
		async_answer_0(callid, rc != EOK ? rc : ENOENT);
		async_answer_1(rid, rc != EOK ? rc : ENOENT, 0);
		return;
found:
		async_data_read_finalize(callid, d_info.d_name,
					 str_size(d_info.d_name) + 1);
		bytes = ((pos - spos) + 1);
	} else {
		struct mfs_sb_info *sbi = mnode->instance->sbi;

		if (pos >= (size_t) ino_i->i_size) {
			/*Trying to read beyond the end of file*/
			bytes = 0;
			(void) async_data_read_finalize(callid, NULL, 0);
			goto out_success;
		}

		bytes = min(len, sbi->block_size - pos % sbi->block_size);
		bytes = min(bytes, ino_i->i_size - pos);

		uint32_t zone;
		block_t *b;

		rc = read_map(&zone, mnode, pos);
		on_error(rc, goto out_error);

		if (zone == 0) {
			/*sparse file*/
			uint8_t *buf = malloc(sbi->block_size);
			if (!buf) {
				rc = ENOMEM;
				goto out_error;
			}
			memset(buf, 0, sizeof(sbi->block_size));
			async_data_read_finalize(callid,
						 buf + pos % sbi->block_size, bytes);
			free(buf);
			goto out_success;
		}

		rc = block_get(&b, handle, zone, BLOCK_FLAGS_NONE);
		on_error(rc, goto out_error);

		async_data_read_finalize(callid, b->data +
					 pos % sbi->block_size, bytes);

		rc = block_put(b);
		if (rc != EOK) {
			mfs_node_put(fn);
			async_answer_0(rid, rc);
			return;
		}
	}
out_success:
	rc = mfs_node_put(fn);
	async_answer_1(rid, rc, (sysarg_t)bytes);
	return;
out_error:
	;
	int tmp = mfs_node_put(fn);
	async_answer_0(callid, tmp != EOK ? tmp : rc);
	async_answer_0(rid, tmp != EOK ? tmp : rc);
}

void
mfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	aoff64_t pos = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request),
					       IPC_GET_ARG4(*request));

	fs_node_t *fn;
	int r;
	int flags = BLOCK_FLAGS_NONE;

	r = mfs_node_get(&fn, handle, index);
	if (r != EOK) {
		async_answer_0(rid, r);
		return;
	}

	if (!fn) {
		async_answer_0(rid, ENOENT);
		return;
	}

	ipc_callid_t callid;
	size_t len;

	if (!async_data_write_receive(&callid, &len)) {
		r = EINVAL;
		goto out_err;
	}

	struct mfs_node *mnode = fn->data;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	const size_t bs = sbi->block_size;
	size_t bytes = min(len, bs - pos % bs);
	size_t boundary = ROUND_UP(ino_i->i_size, bs);
	uint32_t block;

	if (bytes == bs)
		flags = BLOCK_FLAGS_NOREAD;

	if (pos < boundary) {
		r = read_map(&block, mnode, pos);
		on_error(r, goto out_err);

		if (block == 0) {
			/*Writing in a sparse block*/
			r = mfs_alloc_zone(mnode->instance, &block);
			on_error(r, goto out_err);
			flags = BLOCK_FLAGS_NOREAD;
		}
	} else {
		uint32_t dummy;

		r = mfs_alloc_zone(mnode->instance, &block);
		on_error(r, goto out_err);

		r = write_map(mnode, pos, block, &dummy);
		on_error(r, goto out_err);
	}

	block_t *b;
	r = block_get(&b, handle, block, flags);
	on_error(r, goto out_err);

	async_data_write_finalize(callid, b->data + pos % bs, bytes);
	b->dirty = true;

	r = block_put(b);
	if (r != EOK) {
		mfs_node_put(fn);
		async_answer_0(rid, r);
		return;
	}

	ino_i->i_size = pos + bytes;
	ino_i->dirty = true;
	r = mfs_node_put(fn);
	async_answer_2(rid, r, bytes, pos + bytes);
	return;

out_err:
	mfs_node_put(fn);
	async_answer_0(callid, r);
	async_answer_0(rid, r);
}

void
mfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t handle = (devmap_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	fs_node_t *fn;
	int r;

	r = mfs_node_get(&fn, handle, index);
	if (r != EOK) {
		async_answer_0(rid, r);
		return;
	}
	if (!fn) {
		async_answer_0(rid, ENOENT);
		return;
	}

	/*Destroy the inode*/
	r = mfs_destroy_node(fn);
	async_answer_0(rid, r);
}

static int
mfs_destroy_node(fs_node_t *fn)
{
	struct mfs_node *mnode = fn->data;
	bool has_children;
	int r;

	mfsdebug("mfs_destroy_node %d\n", mnode->ino_i->index);

	r = mfs_has_children(&has_children, fn);
	on_error(r, return r);

	assert(!has_children);

	if (mnode->ino_i->i_nlinks > 0) {
		mfsdebug("nlinks = %d", mnode->ino_i->i_nlinks);
		r = EOK;
		goto out;
	}

	/*Free the entire inode content*/
	r = inode_shrink(mnode, mnode->ino_i->i_size);
	on_error(r, return r);
	r = mfs_free_inode(mnode->instance, mnode->ino_i->index);
	on_error(r, return r);

out:
	free(mnode->ino_i);
	free(mnode);
	return r;
}

void
mfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	aoff64_t size = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request),
						IPC_GET_ARG4(*request));
	fs_node_t *fn;
	int r;

	r = mfs_node_get(&fn, handle, index);
	if (r != EOK) {
		async_answer_0(rid, r);
		return;
	}

	if (!fn) {
		async_answer_0(rid, r);
		return;
	}

	struct mfs_node *mnode = fn->data;
	struct mfs_ino_info *ino_i = mnode->ino_i;

	if (ino_i->i_size == size)
		r = EOK;
	else
		r = inode_shrink(mnode, ino_i->i_size - size);

	async_answer_0(rid, r);
	mfs_node_put(fn);
}

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
	bool rc = true;
	*longfilenames = false;

	if (magic == MFS_MAGIC_V1 || magic == MFS_MAGIC_V1R) {
		*native = magic == MFS_MAGIC_V1;
		*version = MFS_VERSION_V1;
	} else if (magic == MFS_MAGIC_V1L || magic == MFS_MAGIC_V1LR) {
		*native = magic == MFS_MAGIC_V1L;
		*version = MFS_VERSION_V1;
		*longfilenames = true;
	} else if (magic == MFS_MAGIC_V2 || magic == MFS_MAGIC_V2R) {
		*native = magic == MFS_MAGIC_V2;
		*version = MFS_VERSION_V2;
	} else if (magic == MFS_MAGIC_V2L || magic == MFS_MAGIC_V2LR) {
		*native = magic == MFS_MAGIC_V2L;
		*version = MFS_VERSION_V2;
		*longfilenames = true;
	} else if (magic == MFS_MAGIC_V3 || magic == MFS_MAGIC_V3R) {
		*native = magic == MFS_MAGIC_V3;
		*version = MFS_VERSION_V3;
	} else
		rc = false;

	return rc;
}

void
mfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	async_answer_0(rid, EOK);
}

void
mfs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&mfs_libfs_ops, mfs_reg.fs_handle, rid, request);
}

void
mfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);

	fs_node_t *fn;
	int rc = mfs_node_get(&fn, devmap, index);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		async_answer_0(rid, ENOENT);
		return;
	}

	struct mfs_node *mnode = fn->data;
	mnode->ino_i->dirty = true;

	rc = mfs_node_put(fn);
	async_answer_0(rid, rc);
}

/**
 * @}
 */

