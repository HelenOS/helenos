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

#include <assert.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

struct mfs_dentry_info *
read_directory_entry(struct mfs_node *mnode, unsigned index)
{
	const struct mfs_instance *inst = mnode->instance;
	const struct mfs_sb_info *sbi = inst->sbi;
	const bool longnames = sbi->long_names;
	uint32_t block;
	block_t *b;

	struct mfs_dentry_info *d_info = malloc(sizeof *d_info);

	if (!d_info)
		return NULL;

	int r = read_map(&block, mnode, index * sbi->dirsize);

	if (r != EOK || block == 0)
		goto out_err;

	r = block_get(&b, inst->handle, block, BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out_err;

	if (sbi->fs_version == MFS_VERSION_V3) {
		struct mfs3_dentry *d3;

		d3 = b->data;
		d_info->d_inum = conv32(sbi->native, d3->d_inum);
		memcpy(d_info->d_name, d3->d_name, MFS3_MAX_NAME_LEN);
	} else {
		const int namelen = longnames ? MFS_L_MAX_NAME_LEN :
					MFS_MAX_NAME_LEN;

		struct mfs_dentry *d;

		d = b->data;
		d_info->d_inum = conv16(sbi->native, d->d_inum);
		memcpy(d_info->d_name, d->d_name, namelen);
	}

	block_put(b);

	d_info->dirty = false;
	d_info->index = index;
	d_info->node = mnode;
	return d_info;

out_err:
	free(d_info);
	return NULL;
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
	if (r != EOK)
		goto out;

	r = block_get(&b, mnode->instance->handle, block, BLOCK_FLAGS_NONE);
	if (r != EOK)
		goto out;

	const size_t name_len = str_size(d_info->d_name);
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
	block_put(b);

out:
	return r;
}

/**
 * @}
 */ 

