#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

static int
find_free_bit_and_set(bitchunk_t *b, const int bsize, const bool native);

int
mfs_alloc_bit(struct mfs_instance *inst, uint32_t *idx, bmap_id_t bid)
{
	struct mfs_sb_info *sbi;
	unsigned start_block;
	unsigned long nblocks;
	unsigned *search, i;
	int r, freebit;

	assert(inst != NULL);
	sbi = inst->sbi;
	assert(sbi != NULL);

	if (bid == BMAP_ZONE) {
		search = &sbi->zsearch;
		start_block = 2 + sbi->ibmap_blocks;
		nblocks = sbi->zbmap_blocks;
	} else {
		/*bid == BMAP_INODE*/
		search = &sbi->isearch;
		start_block = 2;
		nblocks = sbi->ibmap_blocks;
	}

	block_t *b;

retry:

	for (i = *search; i < nblocks; ++i) {
		r = block_get(&b, inst->handle, i, BLOCK_FLAGS_NONE);

		if (r != EOK)
			goto out;

		freebit = find_free_bit_and_set(b->data, sbi->block_size,
						sbi->native);
		if (freebit == -1) {
			/*No free bit in this block*/
			block_put(b);
			continue;
		}

		/*Free bit found, compute real index*/
		*idx = (freebit + sbi->block_size * 8 * i);
		*search = i;
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
find_free_bit_and_set(bitchunk_t *b, const int bsize, const bool native)
{
	int r = -1;
	unsigned i, j;
	bitchunk_t chunk;

	for (i = 0; i < bsize / sizeof(uint32_t); ++i, ++b) {
		if (~*b) {
			/*No free bit in this chunk*/
			continue;
		}

		chunk = conv32(native, *b);

		for (j = 0; j < 32; ++j) {
			if (chunk & (1 << j)) {
				r = i * 32 + j;
				chunk |= 1 << j;
				*b = conv32(native, chunk);
				goto found;
			}
		}
	}
found:
	return r;
}

