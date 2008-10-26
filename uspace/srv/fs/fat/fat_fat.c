/*
 * Copyright (c) 2008 Jakub Jermar
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
 * @file	fat_fat.c
 * @brief	Functions that manipulate the file allocation tables.
 */

#include "fat_fat.h"
#include "fat_dentry.h"
#include "fat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <errno.h>
#include <byteorder.h>
#include <align.h>
#include <assert.h>

block_t *
_fat_block_get(dev_handle_t dev_handle, fat_cluster_t firstc, off_t offset)
{
	block_t *bb;
	block_t *b;
	unsigned bps;
	unsigned spc;
	unsigned rscnt;		/* block address of the first FAT */
	unsigned fatcnt;
	unsigned rde;
	unsigned rds;		/* root directory size */
	unsigned sf;
	unsigned ssa;		/* size of the system area */
	unsigned clusters;
	fat_cluster_t clst = firstc;
	unsigned i;

	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	spc = FAT_BS(bb)->spc;
	rscnt = uint16_t_le2host(FAT_BS(bb)->rscnt);
	fatcnt = FAT_BS(bb)->fatcnt;
	rde = uint16_t_le2host(FAT_BS(bb)->root_ent_max);
	sf = uint16_t_le2host(FAT_BS(bb)->sec_per_fat);
	block_put(bb);

	rds = (sizeof(fat_dentry_t) * rde) / bps;
	rds += ((sizeof(fat_dentry_t) * rde) % bps != 0);
	ssa = rscnt + fatcnt * sf + rds;

	if (firstc == FAT_CLST_ROOT) {
		/* root directory special case */
		assert(offset < rds);
		b = block_get(dev_handle, rscnt + fatcnt * sf + offset, bps);
		return b;
	}

	clusters = offset / spc; 
	for (i = 0; i < clusters; i++) {
		unsigned fsec;	/* sector offset relative to FAT1 */
		unsigned fidx;	/* FAT1 entry index */

		assert(clst >= FAT_CLST_FIRST && clst < FAT_CLST_BAD);
		fsec = (clst * sizeof(fat_cluster_t)) / bps;
		fidx = clst % (bps / sizeof(fat_cluster_t));
		/* read FAT1 */
		b = block_get(dev_handle, rscnt + fsec, bps);
		clst = uint16_t_le2host(((fat_cluster_t *)b->data)[fidx]);
		assert(clst != FAT_CLST_BAD);
		assert(clst < FAT_CLST_LAST1);
		block_put(b);
	}

	b = block_get(dev_handle, ssa + (clst - FAT_CLST_FIRST) * spc +
	    offset % spc, bps);

	return b;
}

/** Return number of blocks allocated to a file.
 *
 * @param dev_handle	Device handle of the device with the file.
 * @param firstc	First cluster of the file.
 * @param lastc		If non-NULL, output argument holding the
 *			last cluster.
 *
 * @return		Number of blocks allocated to the file.
 */
uint16_t 
_fat_blcks_get(dev_handle_t dev_handle, fat_cluster_t firstc,
    fat_cluster_t *lastc)
{
	block_t *bb;
	block_t *b;
	unsigned bps;
	unsigned spc;
	unsigned rscnt;		/* block address of the first FAT */
	unsigned clusters = 0;
	fat_cluster_t clst = firstc;

	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	spc = FAT_BS(bb)->spc;
	rscnt = uint16_t_le2host(FAT_BS(bb)->rscnt);
	block_put(bb);

	if (firstc == FAT_CLST_RES0) {
		/* No space allocated to the file. */
		if (lastc)
			*lastc = firstc;
		return 0;
	}

	while (clst < FAT_CLST_LAST1) {
		unsigned fsec;	/* sector offset relative to FAT1 */
		unsigned fidx;	/* FAT1 entry index */

		assert(clst >= FAT_CLST_FIRST);
		if (lastc)
			*lastc = clst;		/* remember the last cluster */
		fsec = (clst * sizeof(fat_cluster_t)) / bps;
		fidx = clst % (bps / sizeof(fat_cluster_t));
		/* read FAT1 */
		b = block_get(dev_handle, rscnt + fsec, bps);
		clst = uint16_t_le2host(((fat_cluster_t *)b->data)[fidx]);
		assert(clst != FAT_CLST_BAD);
		block_put(b);
		clusters++;
	}

	if (lastc)
		*lastc = clst;
	return clusters * spc;
}

uint16_t fat_bps_get(dev_handle_t dev_handle)
{
	block_t *bb;
	uint16_t bps;
	
	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	assert(bb != NULL);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	block_put(bb);

	return bps;
}

/** Fill the gap between EOF and a new file position.
 *
 * @param nodep		FAT node with the gap.
 * @param mcl		First cluster in an independent cluster chain that will
 *			be later appended to the end of the node's own cluster
 *			chain. If pos is still in the last allocated cluster,
 *			this argument is ignored.
 * @param pos		Position in the last node block.
 */
void fat_fill_gap(fat_node_t *nodep, fat_cluster_t mcl, off_t pos)
{
	uint16_t bps;
	unsigned spc;
	block_t *bb, *b;
	off_t o, boundary;

	bb = block_get(nodep->idx->dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	spc = FAT_BS(bb)->spc;
	block_put(bb);
	
	boundary = ROUND_UP(nodep->size, bps * spc);

	/* zero out already allocated space */
	for (o = nodep->size - 1; o < pos && o < boundary;
	    o = ALIGN_DOWN(o + bps, bps)) {
		b = fat_block_get(nodep, o / bps);
		memset(b->data + o % bps, 0, bps - o % bps);
		b->dirty = true;		/* need to sync node */
		block_put(b);
	}
	
	if (o >= pos)
		return;
	
	/* zero out the initial part of the new cluster chain */
	for (o = boundary; o < pos; o += bps) {
		b = _fat_block_get(nodep->idx->dev_handle, mcl,
		    (o - boundary) / bps);
		memset(b->data, 0, min(bps, pos - o));
		b->dirty = true;		/* need to sync node */
		block_put(b);
	}
}

void
fat_mark_cluster(dev_handle_t dev_handle, unsigned fatno, fat_cluster_t clst,
    fat_cluster_t value)
{
	block_t *bb, *blk;
	uint16_t bps;
	uint16_t rscnt;
	uint16_t sf;
	uint8_t fatcnt;
	fat_cluster_t *cp;

	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	rscnt = uint16_t_le2host(FAT_BS(bb)->rscnt);
	sf = uint16_t_le2host(FAT_BS(bb)->sec_per_fat);
	fatcnt = FAT_BS(bb)->fatcnt;
	block_put(bb);

	assert(fatno < fatcnt);
	blk = block_get(dev_handle, rscnt + sf * fatno +
	    (clst * sizeof(fat_cluster_t)) / bps, bps);
	cp = (fat_cluster_t *)blk->data + clst % (bps / sizeof(fat_cluster_t));
	*cp = host2uint16_t_le(value);
	blk->dirty = true;		/* need to sync block */
	block_put(blk);
}

void fat_alloc_shadow_clusters(dev_handle_t dev_handle, fat_cluster_t *lifo,
    unsigned nclsts)
{
	uint8_t fatcnt;
	uint8_t fatno;
	unsigned c;
	block_t *bb;

	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	fatcnt = FAT_BS(bb)->fatcnt;
	block_put(bb);
	
	for (fatno = FAT1 + 1; fatno < fatcnt; fatno++) {
		for (c = 0; c < nclsts; c++) {
			fat_mark_cluster(dev_handle, fatno, lifo[c],
			    c == 0 ? FAT_CLST_LAST1 : lifo[c - 1]);
		}
	}
}

int
fat_alloc_clusters(dev_handle_t dev_handle, unsigned nclsts, fat_cluster_t *mcl,
    fat_cluster_t *lcl)
{
	uint16_t bps;
	uint16_t rscnt;
	uint16_t sf;
	block_t *bb, *blk;
	fat_cluster_t *lifo;	/* stack for storing free cluster numbers */ 
	unsigned found = 0;	/* top of the free cluster number stack */
	unsigned b, c, cl; 

	lifo = (fat_cluster_t *) malloc(nclsts * sizeof(fat_cluster_t));
	if (lifo)
		return ENOMEM;
	
	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	rscnt = uint16_t_le2host(FAT_BS(bb)->rscnt);
	sf = uint16_t_le2host(FAT_BS(bb)->sec_per_fat);
	block_put(bb);
	
	/*
	 * Search FAT1 for unused clusters.
	 */
	for (b = 0, cl = 0; b < sf; blk++) {
		blk = block_get(dev_handle, rscnt + b, bps);
		for (c = 0; c < bps / sizeof(fat_cluster_t); c++, cl++) {
			fat_cluster_t *clst = (fat_cluster_t *)blk->data + c;
			if (uint16_t_le2host(*clst) == FAT_CLST_RES0) {
				/*
				 * The cluster is free. Put it into our stack
				 * of found clusters and mark it as non-free.
				 */
				lifo[found] = cl;
				*clst = (found == 0) ?
				    host2uint16_t_le(FAT_CLST_LAST1) :
				    host2uint16_t_le(lifo[found - 1]);
				blk->dirty = true;	/* need to sync block */
				if (++found == nclsts) {
					/* we are almost done */
					block_put(blk);
					/* update the shadow copies of FAT */
					fat_alloc_shadow_clusters(dev_handle,
					    lifo, nclsts);
					*mcl = lifo[found - 1];
					*lcl = lifo[0];
					free(lifo);
					return EOK;
				}
			}
		}
		block_put(blk);
	}

	/*
	 * We could not find enough clusters. Now we need to free the clusters
	 * we have allocated so far.
	 */
	while (found--)
		fat_mark_cluster(dev_handle, FAT1, lifo[found], FAT_CLST_RES0);
	
	free(lifo);
	return ENOSPC;
}

void fat_append_clusters(fat_node_t *nodep, fat_cluster_t mcl)
{
	block_t *bb;
	fat_cluster_t lcl;
	uint8_t fatcnt, fatno;

	if (_fat_blcks_get(nodep->idx->dev_handle, nodep->firstc, &lcl) == 0) {
		nodep->firstc = host2uint16_t_le(mcl);
		nodep->dirty = true;		/* need to sync node */
		return;
	}

	bb = block_get(nodep->idx->dev_handle, BS_BLOCK, BS_SIZE);
	fatcnt = FAT_BS(bb)->fatcnt;
	block_put(bb);

	for (fatno = FAT1; fatno < fatcnt; fatno++)
		fat_mark_cluster(nodep->idx->dev_handle, fatno, lcl, mcl);
}

/**
 * @}
 */ 
