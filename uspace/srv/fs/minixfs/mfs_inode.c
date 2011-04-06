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
#include <assert.h>
#include <errno.h>
#include <mem.h>
#include "mfs.h"
#include "mfs_utils.h"

static int
mfs_write_inode_raw(struct mfs_node *mnode);

static int
mfs2_write_inode_raw(struct mfs_node *mnode);

static struct mfs_ino_info *
mfs_read_inode_raw(const struct mfs_instance *instance, uint16_t inum);

static struct mfs_ino_info *
mfs2_read_inode_raw(const struct mfs_instance *instance, uint32_t inum);


int
get_inode(struct mfs_instance *inst, struct mfs_ino_info **ino_i,
				fs_index_t index)
{
	struct mfs_sb_info *sbi = inst->sbi;

	if (sbi->fs_version == MFS_VERSION_V1) {
		/*Read a MFS V1 inode*/
		*ino_i = mfs_read_inode_raw(inst, index);
	} else {
		/*Read a MFS V2/V3 inode*/
		*ino_i = mfs2_read_inode_raw(inst, index);
	}

	if (*ino_i == NULL)
		return -1;

	return EOK;
}

static struct mfs_ino_info *
mfs_read_inode_raw(const struct mfs_instance *instance, uint16_t inum)
{
	struct mfs_inode *ino = NULL;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;

	sbi = instance->sbi;
	assert(sbi);
	
	const int ino_off = inum % sbi->ino_per_block;
	const size_t ino_size = sizeof(struct mfs_inode);

	ino_i = malloc(sizeof(*ino_i));
	ino = malloc(ino_size);

	if (!ino || !ino_i)
		goto out_err;

	const int itable_off = sbi->itable_off;

	if (block_get(&b, instance->handle,
			itable_off + inum / sbi->ino_per_block,
			BLOCK_FLAGS_NONE) != EOK)
		goto out_err;

	memcpy(ino, ((uint8_t *) b->data) + ino_off * ino_size, ino_size);

	ino_i->i_mode = conv16(sbi->native, ino->i_mode);
	ino_i->i_uid = conv16(sbi->native, ino->i_uid);
	ino_i->i_size = conv32(sbi->native, ino->i_size);
	ino_i->i_mtime = conv32(sbi->native, ino->i_mtime);
	ino_i->i_nlinks = ino->i_nlinks;

	for (i = 0; i < V1_NR_DIRECT_ZONES; ++i)
		ino_i->i_dzone[i] = conv16(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V1_NR_INDIRECT_ZONES; ++i)
		ino_i->i_izone[i] = conv16(sbi->native, ino->i_izone[i]);

	block_put(b);
	free(ino);
	ino_i->dirty = false;

	return ino_i;

out_err:
	if (ino)
		free(ino);
	if (ino_i)
		free(ino_i);
	return NULL;
}

static struct mfs_ino_info *
mfs2_read_inode_raw(const struct mfs_instance *instance, uint32_t inum)
{
	struct mfs2_inode *ino = NULL;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;

	const size_t ino_size = sizeof(struct mfs2_inode);

	ino = malloc(ino_size);
	ino_i = malloc(sizeof(*ino_i));

	if (!ino || !ino_i)
		goto out_err;

	sbi = instance->sbi;
	assert(sbi);

	const int itable_off = sbi->itable_off;
	const int ino_off = inum % sbi->ino_per_block;

	if (block_get(&b, instance->handle, 
		itable_off + inum / sbi->ino_per_block,
			BLOCK_FLAGS_NONE) != EOK)
		goto out_err;

	memcpy(ino, ((uint8_t *)b->data) + ino_off * ino_size, ino_size);

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

	block_put(b);
	free(ino);
	ino_i->dirty = false;

	return ino_i;

out_err:
	if (ino)
		free(ino);
	if (ino_i)
		free(ino_i);
	return NULL;
}

int
put_inode(struct mfs_node *mnode)
{
	int rc = EOK;

	assert(mnode);
	assert(mnode->ino_i);

	if (!mnode->ino_i->dirty)
		goto out;

	struct mfs_instance *inst = mnode->instance;
	assert(inst);
	struct mfs_sb_info *sbi = inst->sbi;
	assert(sbi);

	if (sbi->fs_version == MFS_VERSION_V1)
		rc = mfs_write_inode_raw(mnode);
	else
		rc = mfs2_write_inode_raw(mnode);

out:
	return rc;
}

static int
mfs_write_inode_raw(struct mfs_node *mnode)
{
	int i, r;
	block_t *b;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_sb_info *sbi = mnode->instance->sbi;

	const int itable_off = sbi->itable_off;
	const int ino_off = ino_i->index % sbi->ino_per_block;
	const bool native = sbi->native;

	r = block_get(&b, mnode->instance->handle,
				itable_off + ino_i->index / sbi->ino_per_block,
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
	block_put(b);

	ino_i->dirty = false;
out:
	return r;
}

static int
mfs2_write_inode_raw(struct mfs_node *mnode)
{
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	block_t *b;
	int i, r;

	const int itable_off = sbi->itable_off;
	const int ino_off = ino_i->index % sbi->ino_per_block;
	const bool native = sbi->native;
	
	r = block_get(&b, mnode->instance->handle,
				itable_off + ino_i->index / sbi->ino_per_block,
				BLOCK_FLAGS_NONE);

	if (r != EOK)
		goto out;

	struct mfs2_inode *ino2 = b->data;
	ino2 += ino_off;

	ino2->i_mode = conv16(native, ino_i->i_mode);
	ino2->i_nlinks = conv16(native, ino_i->i_mode);
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
	block_put(b);
	ino_i->dirty = false;

out:
	return r;
}

int
inode_grow(struct mfs_node *mnode, size_t size_grow)
{
	unsigned i;

	if (size_grow == 0)
		return EOK;

	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	const int bs = sbi->block_size;

	const uint32_t old_size = ino_i->i_size;
	const uint32_t new_size = old_size + size_grow;

	/*Compute the number of zones to add to the inode*/
	unsigned zones_to_add = 0;
	if ((old_size % (bs - 1)) == 0)
		zones_to_add++;

	zones_to_add += (new_size - old_size) / bs;

	/*Compute the start zone*/
	unsigned start_zone = old_size / bs;
	start_zone += (old_size % bs) != 0;

	int r;
	for (i = 0; i < zones_to_add; ++i) {
		uint32_t new_zone;
		uint32_t dummy;

		r = mfs_alloc_bit(mnode->instance, &new_zone, BMAP_ZONE);
		if (r != EOK)
			return r;

		r = write_map(mnode, (start_zone + i) * sbi->block_size,
				new_zone, &dummy);
		if (r != EOK)
			return r;

		ino_i->i_size += bs;
		ino_i->dirty = true;
	}
	return EOK;
}

/**
 * @}
 */ 

