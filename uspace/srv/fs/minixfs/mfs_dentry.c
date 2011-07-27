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

#include "mfs.h"

int
read_dentry(struct mfs_node *mnode,
		     struct mfs_dentry_info *d_info, unsigned index)
{
	const struct mfs_instance *inst = mnode->instance;
	const struct mfs_sb_info *sbi = inst->sbi;
	const bool longnames = sbi->long_names;
	uint32_t block;
	block_t *b;

	int r = read_map(&block, mnode, index * sbi->dirsize);
	on_error(r, goto out_err);

	if (block == 0) {
		/*End of the dentries list*/
		r = EOK;
		goto out_err;
	}

	r = block_get(&b, inst->handle, block, BLOCK_FLAGS_NONE);
	on_error(r, goto out_err);

	unsigned dentries_per_zone = sbi->block_size / sbi->dirsize;
	unsigned dentry_off = index % dentries_per_zone;

	if (sbi->fs_version == MFS_VERSION_V3) {
		struct mfs3_dentry *d3;

		d3 = b->data + (dentry_off * MFS3_DIRSIZE);

		d_info->d_inum = conv32(sbi->native, d3->d_inum);
		memcpy(d_info->d_name, d3->d_name, MFS3_MAX_NAME_LEN);
	} else {
		const int namelen = longnames ? MFS_L_MAX_NAME_LEN :
				    MFS_MAX_NAME_LEN;

		struct mfs_dentry *d;

		d = b->data + dentry_off * (longnames ? MFSL_DIRSIZE :
					    MFS_DIRSIZE);
		d_info->d_inum = conv16(sbi->native, d->d_inum);
		memcpy(d_info->d_name, d->d_name, namelen);
	}

	r = block_put(b);

	d_info->index = index;
	d_info->node = mnode;

out_err:
	return r;
}

int
write_dentry(struct mfs_dentry_info *d_info)
{
	struct mfs_node *mnode = d_info->node;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	const unsigned d_off_bytes = d_info->index * sbi->dirsize;
	const unsigned dirs_per_block = sbi->block_size / sbi->dirsize;
	block_t *b;
	uint32_t block;
	int r;

	r = read_map(&block, mnode, d_off_bytes);
	on_error(r, goto out);

	r = block_get(&b, mnode->instance->handle, block, BLOCK_FLAGS_NONE);
	on_error(r, goto out);

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

int
remove_dentry(struct mfs_node *mnode, const char *d_name)
{
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_dentry_info d_info;
	int r;

	const size_t name_len = str_size(d_name);

	if (name_len > sbi->max_name_len)
		return ENAMETOOLONG;

	/*Search the directory entry to be removed*/
	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize ; ++i) {
		r = read_dentry(mnode, &d_info, i);
		on_error(r, return r);

		if (!bcmp(d_info.d_name, d_name, name_len)) {
			d_info.d_inum = 0;
			r = write_dentry(&d_info);
			return r;
		}
	}

	return ENOENT;
}

int
insert_dentry(struct mfs_node *mnode, const char *d_name, fs_index_t d_inum)
{
	int r;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_dentry_info d_info;
	bool empty_dentry_found = false;

	const size_t name_len = str_size(d_name);

	if (name_len > sbi->max_name_len)
		return ENAMETOOLONG;

	/*Search for an empty dentry*/
	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = read_dentry(mnode, &d_info, i);
		on_error(r, return r);

		if (d_info.d_inum == 0) {
			/*This entry is not used*/
			empty_dentry_found = true;
			break;
		}
	}

	if (!empty_dentry_found) {
		uint32_t b, pos;
		pos = mnode->ino_i->i_size;
		r = read_map(&b, mnode, pos);
		on_error(r, goto out);

		if (b == 0) {
			/*Increase the inode size*/

			uint32_t dummy;
			r = mfs_alloc_zone(mnode->instance, &b);
			on_error(r, goto out);
			r = write_map(mnode, pos, b, &dummy);
			on_error(r, goto out);
		}

		mnode->ino_i->i_size += sbi->dirsize;
		mnode->ino_i->dirty = true;

		r = read_dentry(mnode, &d_info, i);
		on_error(r, goto out);
	}

	d_info.d_inum = d_inum;
	memcpy(d_info.d_name, d_name, name_len);
	d_info.d_name[name_len] = 0;

	r = write_dentry(&d_info);
out:
	return r;
}


/**
 * @}
 */

