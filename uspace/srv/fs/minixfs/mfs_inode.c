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

struct mfs_ino_info *mfs_read_inode_raw(const struct mfs_instance *instance, 
					uint16_t inum)
{
	struct mfs_inode *ino = NULL;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;
	
	const int ino_off = inum % V1_INODES_PER_BLOCK;
	const size_t ino_size = sizeof(struct mfs_inode);

	ino_i = (struct mfs_ino_info *) malloc(sizeof(struct mfs_ino_info));
	ino = (struct mfs_inode *) malloc(ino_size);

	if (!ino || !ino_i)
		goto out_err;

	sbi = instance->sbi;
	assert(sbi);

	const int itable_off = 2 + sbi->ibmap_blocks + sbi->zbmap_blocks;

	if (block_get(&b, instance->handle, itable_off + inum / V1_INODES_PER_BLOCK,
			BLOCK_FLAGS_NONE) != EOK)
		goto out_err;

	memcpy(ino, ((uint8_t *) b->data) + ino_off * ino_size, ino_size);

	ino_i->i_mode = conv16(sbi->native, ino->i_mode);
	ino_i->i_uid = conv16(sbi->native, ino->i_uid);
	ino_i->i_size = conv32(sbi->native, ino->i_size);
	ino_i->i_mtime = conv32(sbi->native, ino->i_mtime);

	for (i = 0; i < V1_NR_DIRECT_ZONES; ++i)
		ino_i->i_dzone[i] = conv16(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V1_NR_INDIRECT_ZONES; ++i)
		ino_i->i_izone[i] = conv16(sbi->native, ino->i_izone[i]);

	block_put(b);

	free(ino);
	return ino_i;

out_err:
	if (ino)
		free(ino);
	if (ino_i)
		free(ino_i);
	return NULL;
}

struct mfs_ino_info *mfs2_read_inode_raw(const struct mfs_instance *instance,
					uint32_t inum)
{
	struct mfs2_inode *ino = NULL;
	struct mfs_ino_info *ino_i = NULL;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;

	const size_t ino_size = sizeof(struct mfs2_inode);

	ino = (struct mfs2_inode *) malloc(ino_size);
	ino_i = (struct mfs_ino_info *) malloc(sizeof(struct mfs_ino_info));

	if (!ino || !ino_i)
		goto out_err;

	sbi = instance->sbi;
	assert(sbi);

	const int itable_off = 2 + sbi->ibmap_blocks + sbi->zbmap_blocks;
	const int ino_off = inum % V3_INODES_PER_BLOCK(sbi->block_size);

	if (block_get(&b, instance->handle, 
		itable_off + inum / V3_INODES_PER_BLOCK(sbi->block_size),
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
	return ino_i;

out_err:
	if (ino)
		free(ino);
	if (ino_i)
		free(ino_i);
	return NULL;
}

/**
 * @}
 */ 

