/*
 * SPDX-FileCopyrightText: 2011 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup mfs
 * @{
 */

#include <stdlib.h>
#include "mfs.h"

static errno_t
mfs_write_inode_raw(struct mfs_node *mnode);

static errno_t
mfs2_write_inode_raw(struct mfs_node *mnode);

static errno_t
mfs_read_inode_raw(const struct mfs_instance *instance,
    struct mfs_ino_info **ino_ptr, uint16_t inum);

static errno_t
mfs2_read_inode_raw(const struct mfs_instance *instance,
    struct mfs_ino_info **ino_ptr, uint32_t inum);

/**Read a MINIX inode from disk
 *
 * @param inst		Pointer to the filesystem instance.
 * @param ino_i		Pointer to the generic MINIX inode
 * 			where the inode content will be stored.
 * @param index		index of the inode to read.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_get_inode(struct mfs_instance *inst, struct mfs_ino_info **ino_i,
    fs_index_t index)
{
	struct mfs_sb_info *sbi = inst->sbi;
	errno_t r;

	if (sbi->fs_version == MFS_VERSION_V1) {
		/* Read a MFS V1 inode */
		r = mfs_read_inode_raw(inst, ino_i, index);
	} else {
		/* Read a MFS V2/V3 inode */
		r = mfs2_read_inode_raw(inst, ino_i, index);
	}

	return r;
}

static errno_t
mfs_read_inode_raw(const struct mfs_instance *instance,
    struct mfs_ino_info **ino_ptr, uint16_t inum)
{
	struct mfs_inode *ino;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;
	errno_t r;

	sbi = instance->sbi;

	/* inode 0 does not exist */
	inum -= 1;

	const int ino_off = inum % sbi->ino_per_block;

	ino_i = malloc(sizeof(*ino_i));

	if (!ino_i) {
		r = ENOMEM;
		goto out_err;
	}

	const int itable_off = sbi->itable_off;

	r = block_get(&b, instance->service_id,
	    itable_off + inum / sbi->ino_per_block,
	    BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out_err;

	ino = b->data + ino_off * sizeof(struct mfs_inode);

	ino_i->i_mode = conv16(sbi->native, ino->i_mode);
	ino_i->i_uid = conv16(sbi->native, ino->i_uid);
	ino_i->i_size = conv32(sbi->native, ino->i_size);
	ino_i->i_mtime = conv32(sbi->native, ino->i_mtime);
	ino_i->i_nlinks = ino->i_nlinks;

	for (i = 0; i < V1_NR_DIRECT_ZONES; ++i)
		ino_i->i_dzone[i] = conv16(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V1_NR_INDIRECT_ZONES; ++i)
		ino_i->i_izone[i] = conv16(sbi->native, ino->i_izone[i]);

	r = block_put(b);
	ino_i->dirty = false;
	*ino_ptr = ino_i;

	return r;

out_err:
	if (ino_i)
		free(ino_i);
	return EOK;
}

static errno_t
mfs2_read_inode_raw(const struct mfs_instance *instance,
    struct mfs_ino_info **ino_ptr, uint32_t inum)
{
	struct mfs2_inode *ino;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;
	errno_t r;

	ino_i = malloc(sizeof(*ino_i));

	if (!ino_i) {
		r = ENOMEM;
		goto out_err;
	}

	sbi = instance->sbi;

	/* inode 0 does not exist */
	inum -= 1;

	const int itable_off = sbi->itable_off;
	const int ino_off = inum % sbi->ino_per_block;

	r = block_get(&b, instance->service_id,
	    itable_off + inum / sbi->ino_per_block,
	    BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out_err;

	ino = b->data + ino_off * sizeof(struct mfs2_inode);

	ino_i->i_mode = conv16(sbi->native, ino->i_mode);
	ino_i->i_nlinks = conv16(sbi->native, ino->i_nlinks);
	ino_i->i_uid = conv16(sbi->native, ino->i_uid);
	ino_i->i_gid = conv16(sbi->native, ino->i_gid);
	ino_i->i_size = conv32(sbi->native, ino->i_size);
	ino_i->i_atime = conv32(sbi->native, ino->i_atime);
	ino_i->i_mtime = conv32(sbi->native, ino->i_mtime);
	ino_i->i_ctime = conv32(sbi->native, ino->i_ctime);

	for (i = 0; i < V2_NR_DIRECT_ZONES; ++i)
		ino_i->i_dzone[i] = conv32(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V2_NR_INDIRECT_ZONES; ++i)
		ino_i->i_izone[i] = conv32(sbi->native, ino->i_izone[i]);

	r = block_put(b);
	ino_i->dirty = false;
	*ino_ptr = ino_i;

	return r;

out_err:
	if (ino_i)
		free(ino_i);
	return EOK;
}

/**Write a MINIX inode on disk (if marked as dirty)
 *
 * @param mnode		Pointer to the generic MINIX inode in memory.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_put_inode(struct mfs_node *mnode)
{
	errno_t rc = EOK;

	if (!mnode->ino_i->dirty)
		goto out;

	struct mfs_instance *inst = mnode->instance;
	struct mfs_sb_info *sbi = inst->sbi;

	if (sbi->fs_version == MFS_VERSION_V1)
		rc = mfs_write_inode_raw(mnode);
	else
		rc = mfs2_write_inode_raw(mnode);

out:
	return rc;
}

static errno_t
mfs_write_inode_raw(struct mfs_node *mnode)
{
	int i;
	errno_t r;
	block_t *b;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_sb_info *sbi = mnode->instance->sbi;

	const uint32_t inum = ino_i->index - 1;
	const int itable_off = sbi->itable_off;
	const int ino_off = inum % sbi->ino_per_block;
	const bool native = sbi->native;

	r = block_get(&b, mnode->instance->service_id,
	    itable_off + inum / sbi->ino_per_block,
	    BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out;

	struct mfs_inode *ino = b->data;
	ino += ino_off;

	ino->i_mode = conv16(native, ino_i->i_mode);
	ino->i_uid = conv16(native, ino_i->i_uid);
	ino->i_gid = ino_i->i_gid;
	ino->i_nlinks = ino_i->i_nlinks;
	ino->i_size = conv32(native, ino_i->i_size);
	ino->i_mtime = conv32(native, ino_i->i_mtime);

	for (i = 0; i < V1_NR_DIRECT_ZONES; ++i)
		ino->i_dzone[i] = conv16(native, ino_i->i_dzone[i]);
	for (i = 0; i < V1_NR_INDIRECT_ZONES; ++i)
		ino->i_izone[i] = conv16(native, ino_i->i_izone[i]);

	b->dirty = true;
	r = block_put(b);

	ino_i->dirty = false;
out:
	return r;
}

static errno_t
mfs2_write_inode_raw(struct mfs_node *mnode)
{
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	block_t *b;
	int i;
	errno_t r;

	const uint32_t inum = ino_i->index - 1;
	const int itable_off = sbi->itable_off;
	const int ino_off = inum % sbi->ino_per_block;
	const bool native = sbi->native;

	r = block_get(&b, mnode->instance->service_id,
	    itable_off + inum / sbi->ino_per_block,
	    BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out;

	struct mfs2_inode *ino2 = b->data;
	ino2 += ino_off;

	ino2->i_mode = conv16(native, ino_i->i_mode);
	ino2->i_nlinks = conv16(native, ino_i->i_nlinks);
	ino2->i_uid = conv16(native, ino_i->i_uid);
	ino2->i_gid = conv16(native, ino_i->i_gid);
	ino2->i_size = conv32(native, ino_i->i_size);
	ino2->i_atime = conv32(native, ino_i->i_atime);
	ino2->i_mtime = conv32(native, ino_i->i_mtime);
	ino2->i_ctime = conv32(native, ino_i->i_ctime);

	for (i = 0; i < V2_NR_DIRECT_ZONES; ++i)
		ino2->i_dzone[i] = conv32(native, ino_i->i_dzone[i]);

	for (i = 0; i < V2_NR_INDIRECT_ZONES; ++i)
		ino2->i_izone[i] = conv32(native, ino_i->i_izone[i]);

	b->dirty = true;
	r = block_put(b);
	ino_i->dirty = false;

out:
	return r;
}

/**Reduce the inode size of a given number of bytes
 *
 * @param mnode		Pointer to the generic MINIX inode in memory.
 * @param size_shrink	Number of bytes that will be subtracted to the inode.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_inode_shrink(struct mfs_node *mnode, size_t size_shrink)
{
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	const size_t bs = sbi->block_size;
	errno_t r;

	if (size_shrink == 0) {
		/* Nothing to be done */
		return EOK;
	}

	const size_t old_size = ino_i->i_size;
	const size_t new_size = ino_i->i_size - size_shrink;

	assert(size_shrink <= old_size);

	ino_i->dirty = true;

	/* Compute the number of zones to free */
	unsigned zones_to_free;

	size_t diff = old_size - new_size;
	zones_to_free = diff / bs;

	if (diff % bs != 0)
		zones_to_free++;

	uint32_t pos = old_size - 1;
	unsigned i;
	for (i = 0; i < zones_to_free; ++i, pos -= bs) {
		uint32_t old_zone;

		r = mfs_write_map(mnode, pos, 0, &old_zone);
		if (r != EOK)
			goto exit_error;

		ino_i->i_size -= bs;

		if (old_zone == 0)
			continue; /* Sparse block */

		r = mfs_free_zone(mnode->instance, old_zone);
		if (r != EOK)
			goto exit_error;
	}

	ino_i->i_size = new_size;

	return mfs_prune_ind_zones(mnode, new_size);

exit_error:
	return r;
}

/**
 * @}
 */
