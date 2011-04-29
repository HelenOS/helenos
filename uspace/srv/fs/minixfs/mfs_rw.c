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
#include "mfs_utils.h"

static int
rw_map_ondisk(uint32_t *b, const struct mfs_node *mnode, int rblock,
				bool write_mode, uint32_t w_block);

static int
reset_zone_content(struct mfs_instance *inst, uint32_t zone);

static int
alloc_zone_and_clear(struct mfs_instance *inst, uint32_t *zone);

static int
read_ind_zone(struct mfs_instance *inst, uint32_t zone, uint32_t **ind_zone);

static int
write_ind_zone(struct mfs_instance *inst, uint32_t zone, uint32_t *ind_zone);


/**Given the position in the file expressed in
 *bytes, this function returns the on-disk block
 *relative to that position.
 *Returns zero if the block does not exist.
 */
int
read_map(uint32_t *b, const struct mfs_node *mnode, uint32_t pos)
{
	int r;
	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	const int block_size = sbi->block_size;

	/*Compute relative block number in file*/
	int rblock = pos / block_size;

	if (mnode->ino_i->i_size < pos) {
		/*Trying to read beyond the end of file*/
		r = EOK;
		*b = 0;
		goto out;
	}

	r = rw_map_ondisk(b, mnode, rblock, false, 0);
out:
	return r;
}

int
write_map(struct mfs_node *mnode, const uint32_t pos, uint32_t new_zone, 
				uint32_t *old_zone)
{
	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	const int block_size = sbi->block_size;

	/*Compute the relative block number in file*/
	int rblock = pos / block_size;

	return rw_map_ondisk(old_zone, mnode, rblock, true, new_zone);
}

int
free_zone(struct mfs_node *mnode, const uint32_t zone)
{
	int r;
	uint32_t old_zone;

	r = rw_map_ondisk(&old_zone, mnode, zone, true, 0);
	if (r != EOK)
		return r;

	if (old_zone > 0)
		r = mfs_free_bit(mnode->instance, old_zone, BMAP_ZONE);

	return r;
}

static int
rw_map_ondisk(uint32_t *b, const struct mfs_node *mnode, int rblock,
				bool write_mode, uint32_t w_block)
{
	int r, nr_direct;
	int ptrs_per_block;
	uint32_t *ind_zone, *ind2_zone;

	assert(mnode);
	struct mfs_ino_info *ino_i = mnode->ino_i;

	assert(ino_i);
	assert(mnode->instance);

	struct mfs_instance *inst = mnode->instance;
	struct mfs_sb_info *sbi = inst->sbi;
	assert(sbi);

	const mfs_version_t fs_version = sbi->fs_version;

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
		if (write_mode) {
			ino_i->i_dzone[rblock] = w_block;
			ino_i->dirty = true;
		}
		return EOK;
	}

	rblock -= nr_direct;

	if (rblock < ptrs_per_block) {
		/*The wanted block is in the single indirect zone chain*/
		if (ino_i->i_izone[0] == 0) {
			if (write_mode) {
				uint32_t zone;
				r = alloc_zone_and_clear(inst, &zone);
				if (r != EOK)
					return r;

				ino_i->i_izone[0] = zone;
				ino_i->dirty = true;
			} else {
				return -1;
			}
		}

		r = read_ind_zone(inst, ino_i->i_izone[0], &ind_zone);
		if (r != EOK)
			return r;

		*b = ind_zone[rblock];
		if (write_mode) {
			ind_zone[rblock] = w_block;
			write_ind_zone(inst, ino_i->i_izone[0], ind_zone);
		}

		goto out_free_ind1;
	}

	rblock -= ptrs_per_block;

	/*The wanted block is in the double indirect zone chain*/

	/*read the first indirect zone of the chain*/
	if (ino_i->i_izone[1] == 0) {
		if (write_mode) {
			uint32_t zone;
			r = alloc_zone_and_clear(inst, &zone);
			if (r != EOK)
				return r;

			ino_i->i_izone[1] = zone;
			ino_i->dirty = true;
		} else
			return -1;
	}

	r = read_ind_zone(inst, ino_i->i_izone[1], &ind_zone);
	if (r != EOK)
		return r;

	/*
	 *Compute the position of the second indirect
	 *zone pointer in the chain.
	 */
	uint32_t ind2_off = rblock / ptrs_per_block;

	/*read the second indirect zone of the chain*/
	if (ind_zone[ind2_off] == 0) {
		if (write_mode) {
			uint32_t zone;
			r = alloc_zone_and_clear(inst, &zone);
			if(r != EOK)
				goto out_free_ind1;
			ind_zone[ind2_off] = zone;
			write_ind_zone(inst, ino_i->i_izone[1], ind_zone);
		} else {
			r = -1;
			goto out_free_ind1;
		}
	}

	r = read_ind_zone(inst, ind_zone[ind2_off], &ind2_zone);
	if (r != EOK)
		goto out_free_ind1;

	*b = ind2_zone[ind2_off % ptrs_per_block];
	if (write_mode) {
		ind2_zone[ind2_off % ptrs_per_block] = w_block;
		write_ind_zone(inst, ind_zone[ind2_off], ind2_zone);
	}

	r = EOK;

	free(ind2_zone);
out_free_ind1:
	free(ind_zone);
	return r;
}


static int
reset_zone_content(struct mfs_instance *inst, uint32_t zone)
{
	block_t *b;
	int r;

	r = block_get(&b, inst->handle, zone, BLOCK_FLAGS_NOREAD);
	if (r != EOK)
		return r;

	memset(b->data, 0, b->size);
	b->dirty = true;
	block_put(b);

	return EOK;
}

static int
alloc_zone_and_clear(struct mfs_instance *inst, uint32_t *zone)
{
	int r;

	r = mfs_alloc_bit(inst, zone, BMAP_ZONE);
	if (r != EOK)
		goto out;

	r = reset_zone_content(inst, *zone);
out:
	return r;
}

static int
read_ind_zone(struct mfs_instance *inst, uint32_t zone, uint32_t **ind_zone)
{
	struct mfs_sb_info *sbi = inst->sbi;
	int r;
	unsigned i;
	block_t *b;
	const int max_ind_zone_ptrs = (MFS_MAX_BLOCKSIZE / sizeof(uint16_t)) *
				sizeof(uint32_t);

	*ind_zone = malloc(max_ind_zone_ptrs);
	if (*ind_zone == NULL)
		return ENOMEM;

	r = block_get(&b, inst->handle, zone, BLOCK_FLAGS_NONE);
	if (r != EOK) {
		free(*ind_zone);
		return r;
	}

	if (sbi->fs_version == MFS_VERSION_V1) {
		uint16_t *src_ptr = b->data;

		for (i = 0; i < sbi->block_size / sizeof(uint16_t); ++i)
			(*ind_zone)[i] = conv16(sbi->native, src_ptr[i]);
	} else {
		uint32_t *src_ptr = b->data;

		for (i = 0; i < sbi->block_size / sizeof(uint32_t); ++i)
			(*ind_zone)[i] = conv32(sbi->native, src_ptr[i]);
	}

	block_put(b);
	
	return EOK;
}

static int
write_ind_zone(struct mfs_instance *inst, uint32_t zone, uint32_t *ind_zone)
{
	struct mfs_sb_info *sbi = inst->sbi;
	int r;
	unsigned i;
	block_t *b;

	r = block_get(&b, inst->handle, zone, BLOCK_FLAGS_NONE);
	if (r != EOK)
		return r;

	if (sbi->fs_version == MFS_VERSION_V1) {
		uint16_t *dest_ptr = b->data;

		for (i = 0; i < sbi->block_size / sizeof(uint16_t); ++i)
			dest_ptr[i] = conv16(sbi->native, ind_zone[i]);
	} else {
		uint32_t *dest_ptr = b->data;

		for (i = 0; i < sbi->block_size / sizeof(uint32_t); ++i)
			dest_ptr[i] = conv32(sbi->native, ind_zone[i]);

	}
	b->dirty = true;
	block_put(b);
	return EOK;
}


/**
 * @}
 */ 

