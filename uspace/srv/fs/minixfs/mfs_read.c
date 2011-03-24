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

static int read_map_ondisk(uint32_t *b, const struct mfs_node *mnode, int rblock);

/*Given the position in the file expressed in
 *bytes, this function returns the on-disk block
 *relative to that position.
 *Returns zero if the block does not exist.
 */
int read_map(uint32_t *b, const struct mfs_node *mnode, const uint32_t pos)
{
	int r;

	assert(mnode);
	assert(mnode->instance);

	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	assert(sbi);

	const int block_size = sbi->block_size;

	/*Compute relative block number in file*/
	int rblock = pos / block_size;

	if (mnode->ino_i->i_size < (int32_t) pos) {
		/*Trying to read beyond the end of file*/
		r = EOK;
		*b = 0;
		goto out;
	}

	r = read_map_ondisk(b, mnode, rblock);
out:
	return r;
}

static int read_map_ondisk(uint32_t *b, const struct mfs_node *mnode, int rblock)
{
	block_t *bi1, *bi2;
	int r, nr_direct;
	int ptrs_per_block;

	assert(mnode);
	const struct mfs_ino_info *ino_i = mnode->ino_i;

	assert(ino_i);
	assert(mnode->instance);

	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	assert(sbi);

	const int fs_version = sbi->fs_version;

	if (fs_version == MFS_VERSION_V1) {
		nr_direct = V1_NR_DIRECT_ZONES;
		ptrs_per_block = MFS_BLOCKSIZE / sizeof(uint16_t);
	} else {
		nr_direct = V2_NR_DIRECT_ZONES;
		ptrs_per_block = sbi->block_size / sizeof(uint32_t);
	}

	/*Check if the wanted block is in the direct zones*/
	if (rblock < nr_direct) {
		*b = ino_i->i_dzone[rblock];
		r = EOK;
		goto out;
	}
	rblock -= nr_direct - 1;

	if (rblock < ptrs_per_block) {
		/*The wanted block is in the single indirect zone chain*/
		if (ino_i->i_izone[0] == 0) {
			r = -1;
			goto out;
		}

		r = read_ind_block(bi2, mnode->instance, ino_i->i_izone[0]);

		if (r != EOK)
			goto out;

		*b = fs_version == MFS_VERSION_V1 ? 
				((uint16_t *) bi1->data)[rblock] :
				((uint32_t *) bi1->data)[rblock];
		goto out_block;
	}

	rblock -= ptrs_per_block - 1;

	/*The wanted block is in the double indirect zone chain*/

	/*read the first indirect zone of the chain*/
	if (ino_i->i_izone[1] == 0) {
		r = -1;
		goto out;
	}
	r = read_ind_block(bi1, mnode->instance, ino_i->i_izone[1]);

	if (r != EOK)
		goto out;

	/*
	 *Compute the position of the second indirect
	 *zone pointer in the chain.
	 */
	uint32_t di_block = rblock / ptrs_per_block;

	/*read the second indirect zone of the chain*/
	if (fs_version == MFS_VERSION_V1) {
		r = read_ind_block(bi2, mnode->instance,
			((uint16_t *) bi1->data)[di_block]);

		if (r != EOK)
			goto out_block;

		*b = ((uint16_t *) bi2->data)[rblock % ptrs_per_block];
	} else {
		r = read_ind_block(bi2, mnode->instance,
			((uint32_t *) bi1->data)[di_block]);

		if (r != EOK)
			goto out_block;

		*b = ((uint32_t *) bi2->data)[rblock % ptrs_per_block];
	}
	r = EOK;
	block_put(bi2);

out_block:
	block_put(bi1);
out:
	return r;
}

/**
 * @}
 */ 

