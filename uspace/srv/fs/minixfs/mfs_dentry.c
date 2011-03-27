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
		d3->d_inum = conv32(sbi->native, d3->d_inum);

		d_info->d_inum = d3->d_inum;
		memcpy(d_info->d_name, d3->d_name, MFS3_MAX_NAME_LEN);
	} else {
		const int namelen = longnames ? MFS_L_MAX_NAME_LEN :
					MFS_MAX_NAME_LEN;

		struct mfs_dentry *d;

		d = b->data;
		d->d_inum = conv16(sbi->native, d->d_inum);

		d_info->d_inum = d->d_inum;
		memcpy(d_info->d_name, d->d_name, namelen);
	}

	block_put(b);

	d_info->dirty = false;
	return d_info;

out_err:
	free(d_info);
	return NULL;
}

/**
 * @}
 */ 

