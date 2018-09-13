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

#include <str.h>
#include "mfs.h"

/**Read a directory entry from disk.
 *
 * @param mnode		Pointer to the directory node.
 * @param d_info	Pointer to a directory entry structure where
 *			the dentry info will be stored.
 * @param index		index of the dentry in the list.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_read_dentry(struct mfs_node *mnode,
    struct mfs_dentry_info *d_info, unsigned index)
{
	const struct mfs_instance *inst = mnode->instance;
	const struct mfs_sb_info *sbi = inst->sbi;
	const bool longnames = sbi->long_names;
	uint32_t block;
	block_t *b;

	errno_t r = mfs_read_map(&block, mnode, index * sbi->dirsize);
	if (r != EOK)
		goto out_err;

	if (block == 0) {
		/* End of the dentries list */
		r = EOK;
		goto out_err;
	}

	r = block_get(&b, inst->service_id, block, BLOCK_FLAGS_NONE);
	if (r != EOK)
		goto out_err;

	unsigned dentries_per_zone = sbi->block_size / sbi->dirsize;
	unsigned dentry_off = index % dentries_per_zone;

	if (sbi->fs_version == MFS_VERSION_V3) {
		struct mfs3_dentry *d3;

		d3 = b->data + (dentry_off * MFS3_DIRSIZE);

		d_info->d_inum = conv32(sbi->native, d3->d_inum);
		memcpy(d_info->d_name, d3->d_name, MFS3_MAX_NAME_LEN);
		d_info->d_name[MFS3_MAX_NAME_LEN] = 0;
	} else {
		const int namelen = longnames ? MFS_L_MAX_NAME_LEN :
		    MFS_MAX_NAME_LEN;

		struct mfs_dentry *d;

		d = b->data + dentry_off * (longnames ? MFSL_DIRSIZE :
		    MFS_DIRSIZE);
		d_info->d_inum = conv16(sbi->native, d->d_inum);
		memcpy(d_info->d_name, d->d_name, namelen);
		d_info->d_name[namelen] = 0;
	}

	r = block_put(b);

	d_info->index = index;
	d_info->node = mnode;

out_err:
	return r;
}

/**Write a directory entry on disk.
 *
 * @param d_info The directory entry to write to disk.
 *
 * @return	 EOK on success or an error code.
 */
errno_t
mfs_write_dentry(struct mfs_dentry_info *d_info)
{
	struct mfs_node *mnode = d_info->node;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	const unsigned d_off_bytes = d_info->index * sbi->dirsize;
	const unsigned dirs_per_block = sbi->block_size / sbi->dirsize;
	block_t *b;
	uint32_t block;
	errno_t r;

	r = mfs_read_map(&block, mnode, d_off_bytes);
	if (r != EOK)
		goto out;

	r = block_get(&b, mnode->instance->service_id, block, BLOCK_FLAGS_NONE);
	if (r != EOK)
		goto out;

	const size_t name_len = sbi->max_name_len;
	uint8_t *ptr = b->data;
	ptr += (d_info->index % dirs_per_block) * sbi->dirsize;

	if (sbi->fs_version == MFS_VERSION_V3) {
		struct mfs3_dentry *dentry;
		dentry = (struct mfs3_dentry *) ptr;

		dentry->d_inum = conv32(sbi->native, d_info->d_inum);
		memcpy(dentry->d_name, d_info->d_name, name_len);
	} else {
		struct mfs_dentry *dentry;
		dentry = (struct mfs_dentry *) ptr;

		dentry->d_inum = conv16(sbi->native, d_info->d_inum);
		memcpy(dentry->d_name, d_info->d_name, name_len);
	}

	b->dirty = true;
	r = block_put(b);

out:
	return r;
}

/**Remove a directory entry from a directory.
 *
 * @param mnode		Pointer to the directory node.
 * @param d_name	Name of the directory entry to delete.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_remove_dentry(struct mfs_node *mnode, const char *d_name)
{
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_dentry_info d_info;
	errno_t r;

	const size_t name_len = str_size(d_name);

	if (name_len > sbi->max_name_len)
		return ENAMETOOLONG;

	/* Search the directory entry to be removed */
	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = mfs_read_dentry(mnode, &d_info, i);
		if (r != EOK)
			return r;

		const size_t d_name_len = str_size(d_info.d_name);

		if (name_len == d_name_len &&
		    memcmp(d_info.d_name, d_name, name_len) == 0) {

			d_info.d_inum = 0;
			r = mfs_write_dentry(&d_info);
			return r;
		}
	}

	return ENOENT;
}

/**Insert a new directory entry in a existing directory.
 *
 * @param mnode		Pointer to the directory node.
 * @param d_name	Name of the new directory entry.
 * @param d_inum	index of the inode that will be pointed by the new dentry.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_insert_dentry(struct mfs_node *mnode, const char *d_name,
    fs_index_t d_inum)
{
	errno_t r;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_dentry_info d_info;
	bool empty_dentry_found = false;

	const size_t name_len = str_size(d_name);

	if (name_len > sbi->max_name_len)
		return ENAMETOOLONG;

	/* Search for an empty dentry */
	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = mfs_read_dentry(mnode, &d_info, i);
		if (r != EOK)
			return r;

		if (d_info.d_inum == 0) {
			/* This entry is not used */
			empty_dentry_found = true;
			break;
		}
	}

	if (!empty_dentry_found) {
		uint32_t b, pos;
		pos = mnode->ino_i->i_size;
		r = mfs_read_map(&b, mnode, pos);
		if (r != EOK)
			goto out;

		if (b == 0) {
			/* Increase the inode size */

			uint32_t dummy;
			r = mfs_alloc_zone(mnode->instance, &b);
			if (r != EOK)
				goto out;
			r = mfs_write_map(mnode, pos, b, &dummy);
			if (r != EOK) {
				mfs_free_zone(mnode->instance, b);
				goto out;
			}
		}

		mnode->ino_i->i_size += sbi->dirsize;
		mnode->ino_i->dirty = true;

		d_info.index = i;
		d_info.node = mnode;
	}

	d_info.d_inum = d_inum;
	memcpy(d_info.d_name, d_name, name_len);
	if (name_len < sbi->max_name_len)
		d_info.d_name[name_len] = 0;

	r = mfs_write_dentry(&d_info);
out:
	return r;
}

/**
 * @}
 */
