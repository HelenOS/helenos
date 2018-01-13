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
#include "mfs.h"

static int
find_free_bit_and_set(bitchunk_t *b, const int bsize,
    const bool native, unsigned start_bit);

static errno_t
mfs_free_bit(struct mfs_instance *inst, uint32_t idx, bmap_id_t bid);

static errno_t
mfs_alloc_bit(struct mfs_instance *inst, uint32_t *idx, bmap_id_t bid);

static errno_t
mfs_count_free_bits(struct mfs_instance *inst, bmap_id_t bid, uint32_t *free);


/**Allocate a new inode.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param inum		Pointer to a 32 bit number where the index of
 * 			the new inode will be saved.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_alloc_inode(struct mfs_instance *inst, uint32_t *inum)
{
	errno_t r = mfs_alloc_bit(inst, inum, BMAP_INODE);
	return r;
}

/**Free an inode.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param inum		Number of the inode to free.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_free_inode(struct mfs_instance *inst, uint32_t inum)
{
	return mfs_free_bit(inst, inum, BMAP_INODE);
}

/**Allocate a new zone.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param zone		Pointer to a 32 bit number where the index
 * 			of the zone will be saved.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_alloc_zone(struct mfs_instance *inst, uint32_t *zone)
{
	errno_t r = mfs_alloc_bit(inst, zone, BMAP_ZONE);
	if (r != EOK)
		return r;

	/* Update the cached number of free zones */
	struct mfs_sb_info *sbi = inst->sbi;
	if (sbi->nfree_zones_valid)
		sbi->nfree_zones--;

	*zone += inst->sbi->firstdatazone - 1;
	return r;
}

/**Free a zone.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param zone		Index of the zone to free.
 *
 * @return		EOK on success or an error code.
 */
errno_t
mfs_free_zone(struct mfs_instance *inst, uint32_t zone)
{
	errno_t r;

	zone -= inst->sbi->firstdatazone - 1;

	r = mfs_free_bit(inst, zone, BMAP_ZONE);
	if (r != EOK)
		return r;

	/* Update the cached number of free zones */
	struct mfs_sb_info *sbi = inst->sbi;
	if (sbi->nfree_zones_valid)
		sbi->nfree_zones++;

	return r;
}

/** Count the number of free zones
 *
 * @param inst          Pointer to the instance structure.
 * @param zones         Pointer to the memory location where the result
 *                      will be stored.
 *
 * @return              EOK on success or an error code.
 */
errno_t
mfs_count_free_zones(struct mfs_instance *inst, uint32_t *zones)
{
	return mfs_count_free_bits(inst, BMAP_ZONE, zones);
}

/** Count the number of free inodes
 *
 * @param inst          Pointer to the instance structure.
 * @param zones         Pointer to the memory location where the result
 *                      will be stored.
 *
 * @return              EOK on success or an error code.
 */

errno_t
mfs_count_free_inodes(struct mfs_instance *inst, uint32_t *inodes)
{
	return mfs_count_free_bits(inst, BMAP_INODE, inodes);
}

/** Count the number of free bits in a bitmap
 *
 * @param inst          Pointer to the instance structure.
 * @param bid           Type of the bitmap (inode or zone).
 * @param free          Pointer to the memory location where the result
 *                      will be stores.
 *
 * @return              EOK on success or an error code.
 */
static errno_t
mfs_count_free_bits(struct mfs_instance *inst, bmap_id_t bid, uint32_t *free)
{
	errno_t r;
	unsigned start_block;
	unsigned long nblocks;
	unsigned long nbits;
	unsigned long block;
	unsigned long free_bits = 0;
	bitchunk_t chunk;
	size_t const bitchunk_bits = sizeof(bitchunk_t) * 8;
	block_t *b;
	struct mfs_sb_info *sbi = inst->sbi;

	start_block = MFS_BMAP_START_BLOCK(sbi, bid);
	nblocks = MFS_BMAP_SIZE_BLOCKS(sbi, bid);
	nbits = MFS_BMAP_SIZE_BITS(sbi, bid);

	for (block = 0; block < nblocks; ++block) {
		r = block_get(&b, inst->service_id, block + start_block,
		    BLOCK_FLAGS_NONE);
		if (r != EOK)
			return r;

		size_t i;
		bitchunk_t *data = (bitchunk_t *) b->data;

		/* Read the bitmap block, chunk per chunk,
		 * counting the zero bits.
		 */
		for (i = 0; i < sbi->block_size / sizeof(bitchunk_t); ++i) {
			chunk = conv32(sbi->native, data[i]);

			size_t bit;
			for (bit = 0; bit < bitchunk_bits && nbits > 0;
			    ++bit, --nbits) {
				if (!(chunk & (1 << bit)))
					free_bits++;
			}

			if (nbits == 0)
				break;
		}

		r = block_put(b);
		if (r != EOK)
			return r;
	}

	*free = free_bits;
	assert(nbits == 0);

	return EOK;
}

/**Clear a bit in a bitmap.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param idx		Index of the bit to clear.
 * @param bid		BMAP_ZONE if operating on the zone's bitmap,
 * 			BMAP_INODE if operating on the inode's bitmap.
 *
 * @return		EOK on success or an error code.
 */
static errno_t
mfs_free_bit(struct mfs_instance *inst, uint32_t idx, bmap_id_t bid)
{
	struct mfs_sb_info *sbi;
	errno_t r;
	unsigned start_block;
	unsigned *search;
	block_t *b;

	sbi = inst->sbi;

	start_block = MFS_BMAP_START_BLOCK(sbi, bid);

	if (bid == BMAP_ZONE) {
		search = &sbi->zsearch;
		if (idx > sbi->nzones) {
			printf(NAME ": Error! Trying to free beyond the "
			    "bitmap max size\n");
			return EIO;
		}
	} else {
		/* bid == BMAP_INODE */
		search = &sbi->isearch;
		if (idx > sbi->ninodes) {
			printf(NAME ": Error! Trying to free beyond the "
			    "bitmap max size\n");
			return EIO;
		}
	}

	/* Compute the bitmap block */
	uint32_t block = idx / (sbi->block_size * 8) + start_block;

	r = block_get(&b, inst->service_id, block, BLOCK_FLAGS_NONE);
	if (r != EOK)
		goto out_err;

	/* Compute the bit index in the block */
	idx %= (sbi->block_size * 8);
	bitchunk_t *ptr = b->data;
	bitchunk_t chunk;
	const size_t chunk_bits = sizeof(bitchunk_t) * 8;

	chunk = conv32(sbi->native, ptr[idx / chunk_bits]);
	chunk &= ~(1 << (idx % chunk_bits));
	ptr[idx / chunk_bits] = conv32(sbi->native, chunk);

	b->dirty = true;
	r = block_put(b);

	if (*search > idx)
		*search = idx;

out_err:
	return r;
}

/**Search a free bit in a bitmap and mark it as used.
 *
 * @param inst		Pointer to the filesystem instance.
 * @param idx		Pointer of a 32 bit number where the index
 * 			of the found bit will be stored.
 * @param bid		BMAP_ZONE if operating on the zone's bitmap,
 * 			BMAP_INODE if operating on the inode's bitmap.
 *
 * @return		EOK on success or an error code.
 */
static errno_t
mfs_alloc_bit(struct mfs_instance *inst, uint32_t *idx, bmap_id_t bid)
{
	struct mfs_sb_info *sbi;
	uint32_t limit;
	unsigned long nblocks;
	unsigned *search, i, start_block;
	unsigned bits_per_block;
	errno_t r;
	int freebit;

	sbi = inst->sbi;

	start_block = MFS_BMAP_START_BLOCK(sbi, bid);
	limit = MFS_BMAP_SIZE_BITS(sbi, bid);
	nblocks = MFS_BMAP_SIZE_BLOCKS(sbi, bid);

	if (bid == BMAP_ZONE) {
		search = &sbi->zsearch;
	} else {
		/* bid == BMAP_INODE */
		search = &sbi->isearch;
	}
	bits_per_block = sbi->block_size * 8;

	block_t *b;

retry:

	for (i = *search / bits_per_block; i < nblocks; ++i) {
		r = block_get(&b, inst->service_id, i + start_block,
		    BLOCK_FLAGS_NONE);

		if (r != EOK)
			goto out;

		unsigned tmp = *search % bits_per_block;

		freebit = find_free_bit_and_set(b->data, sbi->block_size,
		    sbi->native, tmp);
		if (freebit == -1) {
			/* No free bit in this block */
			r = block_put(b);
			if (r != EOK)
				goto out;
			continue;
		}

		/* Free bit found in this block, compute the real index */
		*idx = freebit + bits_per_block * i;
		if (*idx > limit) {
			/* Index is beyond the limit, it is invalid */
			r = block_put(b);
			if (r != EOK)
				goto out;
			break;
		}

		*search = *idx;
		b->dirty = true;
		r = block_put(b);
		goto out;
	}

	if (*search > 0) {
		/* Repeat the search from the first bitmap block */
		*search = 0;
		goto retry;
	}

	/* Free bit not found, return error */
	return ENOSPC;

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
	const size_t chunk_bits = sizeof(bitchunk_t) * 8;

	for (i = start_bit / chunk_bits;
	    i < bsize / sizeof(bitchunk_t); ++i) {

		if (!(~b[i])) {
			/* No free bit in this chunk */
			continue;
		}

		chunk = conv32(native, b[i]);

		for (j = 0; j < chunk_bits; ++j) {
			if (!(chunk & (1 << j))) {
				r = i * chunk_bits + j;
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

