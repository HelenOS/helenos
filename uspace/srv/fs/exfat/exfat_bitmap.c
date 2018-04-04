/*
 * Copyright (c) 2011 Oleg Romanenko
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

/**
 * @file	exfat_bitmap.c
 * @brief	Functions that manipulate the Bitmap Table.
 */

#include "exfat_bitmap.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <block.h>
#include <errno.h>
#include <byteorder.h>
#include <align.h>
#include <assert.h>
#include <fibril_synch.h>
#include <mem.h>


errno_t exfat_bitmap_is_free(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t clst)
{
	fs_node_t *fn;
	block_t *b = NULL;
	exfat_node_t *bitmapp;
	uint8_t *bitmap;
	errno_t rc;
	bool alloc;

	clst -= EXFAT_CLST_FIRST;

	rc = exfat_bitmap_get(&fn, service_id);
	if (rc != EOK)
		return rc;
	bitmapp = EXFAT_NODE(fn);

	aoff64_t offset = clst / 8;
	rc = exfat_block_get(&b, bs, bitmapp, offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}
	bitmap = (uint8_t *)b->data;
	alloc = bitmap[offset % BPS(bs)] & (1 << (clst % 8));

	rc = block_put(b);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}
	rc = exfat_node_put(fn);
	if (rc != EOK)
		return rc;

	if (alloc)
		return ENOENT;

	return EOK;
}

errno_t exfat_bitmap_set_cluster(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t clst)
{
	fs_node_t *fn;
	block_t *b = NULL;
	exfat_node_t *bitmapp;
	uint8_t *bitmap;
	errno_t rc;

	clst -= EXFAT_CLST_FIRST;

	rc = exfat_bitmap_get(&fn, service_id);
	if (rc != EOK)
		return rc;
	bitmapp = EXFAT_NODE(fn);

	aoff64_t offset = clst / 8;
	rc = exfat_block_get(&b, bs, bitmapp, offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}
	bitmap = (uint8_t *)b->data;
	bitmap[offset % BPS(bs)] |= (1 << (clst % 8));

	b->dirty = true;
	rc = block_put(b);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}

	return exfat_node_put(fn);
}

errno_t exfat_bitmap_clear_cluster(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t clst)
{
	fs_node_t *fn;
	block_t *b = NULL;
	exfat_node_t *bitmapp;
	uint8_t *bitmap;
	errno_t rc;

	clst -= EXFAT_CLST_FIRST;

	rc = exfat_bitmap_get(&fn, service_id);
	if (rc != EOK)
		return rc;
	bitmapp = EXFAT_NODE(fn);

	aoff64_t offset = clst / 8;
	rc = exfat_block_get(&b, bs, bitmapp, offset / BPS(bs),
	    BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}
	bitmap = (uint8_t *)b->data;
	bitmap[offset % BPS(bs)] &= ~(1 << (clst % 8));

	b->dirty = true;
	rc = block_put(b);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}

	return exfat_node_put(fn);
}

errno_t exfat_bitmap_set_clusters(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t firstc, exfat_cluster_t count)
{
	errno_t rc;
	exfat_cluster_t clst;
	clst = firstc;

	while (clst < firstc + count) {
		rc = exfat_bitmap_set_cluster(bs, service_id, clst);
		if (rc != EOK) {
			if (clst - firstc > 0)
				(void) exfat_bitmap_clear_clusters(bs, service_id,
				    firstc, clst - firstc);
			return rc;
		}
		clst++;
	}
	return EOK;
}

errno_t exfat_bitmap_clear_clusters(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t firstc, exfat_cluster_t count)
{
	errno_t rc;
	exfat_cluster_t clst;
	clst = firstc;

	while (clst < firstc + count) {
		rc = exfat_bitmap_clear_cluster(bs, service_id, clst);
		if (rc != EOK)
			return rc;
		clst++;
	}
	return EOK;
}

errno_t exfat_bitmap_alloc_clusters(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t *firstc, exfat_cluster_t count)
{
	exfat_cluster_t startc, endc;
	startc = EXFAT_CLST_FIRST;

	while (startc < DATA_CNT(bs) + 2) {
		endc = startc;
		while (exfat_bitmap_is_free(bs, service_id, endc) == EOK) {
			if ((endc - startc) + 1 == count) {
				*firstc = startc;
				return exfat_bitmap_set_clusters(bs, service_id, startc, count);
			} else
				endc++;
		}
		startc = endc + 1;
	}
	return ENOSPC;
}


errno_t exfat_bitmap_append_clusters(exfat_bs_t *bs, exfat_node_t *nodep,
    exfat_cluster_t count)
{
	if (nodep->firstc == 0) {
		return exfat_bitmap_alloc_clusters(bs, nodep->idx->service_id,
		    &nodep->firstc, count);
	} else {
		exfat_cluster_t lastc, clst;
		lastc = nodep->firstc + ROUND_UP(nodep->size, BPC(bs)) / BPC(bs) - 1;

		clst = lastc + 1;
		while (exfat_bitmap_is_free(bs, nodep->idx->service_id, clst) == EOK) {
			if (clst - lastc == count) {
				return exfat_bitmap_set_clusters(bs, nodep->idx->service_id,
				    lastc + 1, count);
			} else
				clst++;
		}
		return ENOSPC;
	}
}


errno_t exfat_bitmap_free_clusters(exfat_bs_t *bs, exfat_node_t *nodep,
    exfat_cluster_t count)
{
	exfat_cluster_t lastc;
	lastc = nodep->firstc + ROUND_UP(nodep->size, BPC(bs)) / BPC(bs) - 1;
	lastc -= count;

	return exfat_bitmap_clear_clusters(bs, nodep->idx->service_id, lastc + 1, count);
}


errno_t exfat_bitmap_replicate_clusters(exfat_bs_t *bs, exfat_node_t *nodep)
{
	errno_t rc;
	exfat_cluster_t lastc, clst;
	service_id_t service_id = nodep->idx->service_id;
	lastc = nodep->firstc + ROUND_UP(nodep->size, BPC(bs)) / BPC(bs) - 1;

	for (clst = nodep->firstc; clst < lastc; clst++) {
		rc = exfat_set_cluster(bs, service_id, clst, clst + 1);
		if (rc != EOK)
			return rc;
	}

	return exfat_set_cluster(bs, service_id, lastc, EXFAT_CLST_EOF);
}



/**
 * @}
 */
