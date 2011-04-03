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
#include "mfs.h"
#include "mfs_utils.h"

static int
find_free_bit_and_set(bitchunk_t *b, const int bsize,
				const bool native, unsigned start_bit);

int
mfs_free_bit(struct mfs_instance *inst, uint32_t idx, bmap_id_t bid)
{
	struct mfs_sb_info *sbi;
	int r;
	unsigned start_block;
	block_t *b;

	assert(inst != NULL);
	sbi = inst->sbi;
	assert(sbi != NULL);

	if (bid == BMAP_ZONE) {
		start_block = 2 + sbi->ibmap_blocks;
		if (idx > sbi->nzones) {
			printf(NAME ": Error! Trying to free beyond the" \
					"bitmap max size\n");
			return -1;
		}
	} else {
		/*bid == BMAP_INODE*/
		start_block = 2;
		if (idx > sbi->ninodes) {
			printf(NAME ": Error! Trying to free beyond the" \
					"bitmap max size\n");
			return -1;
		}
	}

	/*Compute the bitmap block*/
	uint32_t block = idx / (sbi->block_size * 8) + start_block;

	r = block_get(&b, inst->handle, block, BLOCK_FLAGS_NONE);
	if (r != EOK)
		goto out_err;

	/*Compute the bit index in the block*/
	idx %= (sbi->block_size * 8);
	bitchunk_t *ptr = b->data;
	bitchunk_t chunk;

	chunk = conv32(sbi->native, ptr[idx / 32]);
	chunk &= ~(1 << (idx % 32));
	ptr[idx / 32] = conv32(sbi->native, chunk);
	b->dirty = true;
	r = EOK;
	block_put(b);

out_err:
	return r;
}

int
mfs_alloc_bit(struct mfs_instance *inst, uint32_t *idx, bmap_id_t bid)
{
	struct mfs_sb_info *sbi;
	uint32_t limit;
	unsigned long nblocks;
	unsigned *search, i, start_block;
	unsigned bits_per_block;
	int r, freebit;

	assert(inst != NULL);
	sbi = inst->sbi;
	assert(sbi != NULL);

	if (bid == BMAP_ZONE) {
		search = &sbi->zsearch;
		start_block = 2 + sbi->ibmap_blocks;
		nblocks = sbi->zbmap_blocks;
		limit = sbi->nzones;
	} else {
		/*bid == BMAP_INODE*/
		search = &sbi->isearch;
		start_block = 2;
		nblocks = sbi->ibmap_blocks;
		limit = sbi->ninodes;
	}
	bits_per_block = sbi->block_size * 8;

	block_t *b;

retry:

	for (i = *search / bits_per_block; i < nblocks; ++i) {
		r = block_get(&b, inst->handle, i + start_block,
				BLOCK_FLAGS_NONE);

		if (r != EOK)
			goto out;

		freebit = find_free_bit_and_set(b->data, sbi->block_size,
						sbi->native, *search);
		if (freebit == -1) {
			/*No free bit in this block*/
			block_put(b);
			continue;
		}

		/*Free bit found, compute real index*/
		*idx = (freebit + sbi->block_size * 8 * i);
		if (*idx > limit) {
			/*Index is beyond the limit, it is invalid*/
			block_put(b);
			break;
		}

		*search = i * bits_per_block + *idx;
		b->dirty = true;
		block_put(b);
		goto found;
	}

	if (*search > 0) {
		/*Repeat the search from the first bitmap block*/
		*search = 0;
		goto retry;
	}

	/*Free bit not found, return error*/
	return ENOSPC;

found:
	r = EOK;

out:
	return r;
}

static int
find_free_bit_and_set(bitchunk_t *b, const int bsize,
				const bool native, unsigned start_bit)
{
	int r = -1;
	unsigned i, j;
	bitchunk_t chunk;

	for (i = start_bit; i < bsize / sizeof(uint32_t); ++i) {
		if (~b[i]) {
			/*No free bit in this chunk*/
			continue;
		}

		chunk = conv32(native, b[i]);

		for (j = 0; j < 32; ++j) {
			if (chunk & (1 << j)) {
				r = i * 32 + j;
				chunk |= 1 << j;
				b[i] = conv32(native, chunk);
				goto found;
			}
		}
	}
found:
	return r;
}

/**
 * @}
 */ 

