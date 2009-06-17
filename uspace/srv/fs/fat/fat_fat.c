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
 * @brief	Functions that manipulate the File Allocation Tables.
 */

#include "fat_fat.h"
#include "fat_dentry.h"
#include "fat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <errno.h>
#include <byteorder.h>
#include <align.h>
#include <assert.h>
#include <fibril_sync.h>
#include <mem.h>

/**
 * The fat_alloc_lock mutex protects all copies of the File Allocation Table
 * during allocation of clusters. The lock does not have to be held durring
 * deallocation of clusters.
 */  
static FIBRIL_MUTEX_INITIALIZE(fat_alloc_lock);

/** Walk the cluster chain.
 *
 * @param bs		Buffer holding the boot sector for the file.
 * @param dev_handle	Device handle of the device with the file.
 * @param firstc	First cluster to start the walk with.
 * @param lastc		If non-NULL, output argument hodling the last cluster number visited.
 * @param max_clusters	Maximum number of clusters to visit.	
 *
 * @return		Number of clusters seen during the walk.
 */
uint16_t 
fat_cluster_walk(fat_bs_t *bs, dev_handle_t dev_handle, fat_cluster_t firstc,
    fat_cluster_t *lastc, uint16_t max_clusters)
{
	block_t *b;
	unsigned bps;
	unsigned rscnt;		/* block address of the first FAT */
	uint16_t clusters = 0;
	fat_cluster_t clst = firstc;

	bps = uint16_t_le2host(bs->bps);
	rscnt = uint16_t_le2host(bs->rscnt);

	if (firstc == FAT_CLST_RES0) {
		/* No space allocated to the file. */
		if (lastc)
			*lastc = firstc;
		return 0;
	}

	while (clst < FAT_CLST_LAST1 && clusters < max_clusters) {
		bn_t fsec;	/* sector offset relative to FAT1 */
		unsigned fidx;	/* FAT1 entry index */

		assert(clst >= FAT_CLST_FIRST);
		if (lastc)
			*lastc = clst;	/* remember the last cluster number */
		fsec = (clst * sizeof(fat_cluster_t)) / bps;
		fidx = clst % (bps / sizeof(fat_cluster_t));
		/* read FAT1 */
		b = block_get(dev_handle, rscnt + fsec, BLOCK_FLAGS_NONE);
		clst = uint16_t_le2host(((fat_cluster_t *)b->data)[fidx]);
		assert(clst != FAT_CLST_BAD);
		block_put(b);
		clusters++;
	}

	if (lastc && clst < FAT_CLST_LAST1)
		*lastc = clst;

	return clusters;
}

/** Read block from file located on a FAT file system.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param dev_handle	Device handle of the file system.
 * @param firstc	First cluster used by the file. Can be zero if the file
 *			is empty.
 * @param bn		Block number.
 * @param flags		Flags passed to libblock.
 *
 * @return		Block structure holding the requested block.
 */
block_t *
_fat_block_get(fat_bs_t *bs, dev_handle_t dev_handle, fat_cluster_t firstc,
    bn_t bn, int flags)
{
	block_t *b;
	unsigned bps;
	unsigned rscnt;		/* block address of the first FAT */
	unsigned rde;
	unsigned rds;		/* root directory size */
	unsigned sf;
	unsigned ssa;		/* size of the system area */
	unsigned clusters, max_clusters;
	fat_cluster_t lastc;

	bps = uint16_t_le2host(bs->bps);
	rscnt = uint16_t_le2host(bs->rscnt);
	rde = uint16_t_le2host(bs->root_ent_max);
	sf = uint16_t_le2host(bs->sec_per_fat);

	rds = (sizeof(fat_dentry_t) * rde) / bps;
	rds += ((sizeof(fat_dentry_t) * rde) % bps != 0);
	ssa = rscnt + bs->fatcnt * sf + rds;

	if (firstc == FAT_CLST_ROOT) {
		/* root directory special case */
		assert(bn < rds);
		b = block_get(dev_handle, rscnt + bs->fatcnt * sf + bn, flags);
		return b;
	}

	max_clusters = bn / bs->spc;
	clusters = fat_cluster_walk(bs, dev_handle, firstc, &lastc,
	    max_clusters);
	assert(clusters == max_clusters);

	b = block_get(dev_handle, ssa + (lastc - FAT_CLST_FIRST) * bs->spc +
	    bn % bs->spc, flags);

	return b;
}

/** Fill the gap between EOF and a new file position.
 *
 * @param bs		Buffer holding the boot sector for nodep.
 * @param nodep		FAT node with the gap.
 * @param mcl		First cluster in an independent cluster chain that will
 *			be later appended to the end of the node's own cluster
 *			chain. If pos is still in the last allocated cluster,
 *			this argument is ignored.
 * @param pos		Position in the last node block.
 */
void fat_fill_gap(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t mcl, off_t pos)
{
	uint16_t bps;
	unsigned spc;
	block_t *b;
	off_t o, boundary;

	bps = uint16_t_le2host(bs->bps);
	spc = bs->spc;
	
	boundary = ROUND_UP(nodep->size, bps * spc);

	/* zero out already allocated space */
	for (o = nodep->size; o < pos && o < boundary;
	    o = ALIGN_DOWN(o + bps, bps)) {
	    	int flags = (o % bps == 0) ?
		    BLOCK_FLAGS_NOREAD : BLOCK_FLAGS_NONE;
		b = fat_block_get(bs, nodep, o / bps, flags);
		memset(b->data + o % bps, 0, bps - o % bps);
		b->dirty = true;		/* need to sync node */
		block_put(b);
	}
	
	if (o >= pos)
		return;
	
	/* zero out the initial part of the new cluster chain */
	for (o = boundary; o < pos; o += bps) {
		b = _fat_block_get(bs, nodep->idx->dev_handle, mcl,
		    (o - boundary) / bps, BLOCK_FLAGS_NOREAD);
		memset(b->data, 0, min(bps, pos - o));
		b->dirty = true;		/* need to sync node */
		block_put(b);
	}
}

/** Get cluster from the first FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param dev_handle	Device handle for the file system.
 * @param clst		Cluster which to get.
 *
 * @return		Value found in the cluster.
 */
fat_cluster_t
fat_get_cluster(fat_bs_t *bs, dev_handle_t dev_handle, fat_cluster_t clst)
{
	block_t *b;
	uint16_t bps;
	uint16_t rscnt;
	fat_cluster_t *cp, value;

	bps = uint16_t_le2host(bs->bps);
	rscnt = uint16_t_le2host(bs->rscnt);

	b = block_get(dev_handle, rscnt + (clst * sizeof(fat_cluster_t)) / bps,
	    BLOCK_FLAGS_NONE);
	cp = (fat_cluster_t *)b->data + clst % (bps / sizeof(fat_cluster_t));
	value = uint16_t_le2host(*cp);
	block_put(b);
	
	return value;
}

/** Set cluster in one instance of FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param dev_handle	Device handle for the file system.
 * @param fatno		Number of the FAT instance where to make the change.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 */
void
fat_set_cluster(fat_bs_t *bs, dev_handle_t dev_handle, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t value)
{
	block_t *b;
	uint16_t bps;
	uint16_t rscnt;
	uint16_t sf;
	fat_cluster_t *cp;

	bps = uint16_t_le2host(bs->bps);
	rscnt = uint16_t_le2host(bs->rscnt);
	sf = uint16_t_le2host(bs->sec_per_fat);

	assert(fatno < bs->fatcnt);
	b = block_get(dev_handle, rscnt + sf * fatno +
	    (clst * sizeof(fat_cluster_t)) / bps, BLOCK_FLAGS_NONE);
	cp = (fat_cluster_t *)b->data + clst % (bps / sizeof(fat_cluster_t));
	*cp = host2uint16_t_le(value);
	b->dirty = true;		/* need to sync block */
	block_put(b);
}

/** Replay the allocatoin of clusters in all shadow instances of FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param dev_handle	Device handle of the file system.
 * @param lifo		Chain of allocated clusters.
 * @param nclsts	Number of clusters in the lifo chain.
 */
void fat_alloc_shadow_clusters(fat_bs_t *bs, dev_handle_t dev_handle,
    fat_cluster_t *lifo, unsigned nclsts)
{
	uint8_t fatno;
	unsigned c;

	for (fatno = FAT1 + 1; fatno < bs->fatcnt; fatno++) {
		for (c = 0; c < nclsts; c++) {
			fat_set_cluster(bs, dev_handle, fatno, lifo[c],
			    c == 0 ? FAT_CLST_LAST1 : lifo[c - 1]);
		}
	}
}

/** Allocate clusters in all copies of FAT.
 *
 * This function will attempt to allocate the requested number of clusters in
 * all instances of the FAT.  The FAT will be altered so that the allocated
 * clusters form an independent chain (i.e. a chain which does not belong to any
 * file yet).
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param dev_handle	Device handle of the file system.
 * @param nclsts	Number of clusters to allocate.
 * @param mcl		Output parameter where the first cluster in the chain
 *			will be returned.
 * @param lcl		Output parameter where the last cluster in the chain
 *			will be returned.
 *
 * @return		EOK on success, a negative error code otherwise.
 */
int
fat_alloc_clusters(fat_bs_t *bs, dev_handle_t dev_handle, unsigned nclsts,
    fat_cluster_t *mcl, fat_cluster_t *lcl)
{
	uint16_t bps;
	uint16_t rscnt;
	uint16_t sf;
	block_t *blk;
	fat_cluster_t *lifo;	/* stack for storing free cluster numbers */ 
	unsigned found = 0;	/* top of the free cluster number stack */
	unsigned b, c, cl; 

	lifo = (fat_cluster_t *) malloc(nclsts * sizeof(fat_cluster_t));
	if (!lifo)
		return ENOMEM;
	
	bps = uint16_t_le2host(bs->bps);
	rscnt = uint16_t_le2host(bs->rscnt);
	sf = uint16_t_le2host(bs->sec_per_fat);
	
	/*
	 * Search FAT1 for unused clusters.
	 */
	fibril_mutex_lock(&fat_alloc_lock);
	for (b = 0, cl = 0; b < sf; b++) {
		blk = block_get(dev_handle, rscnt + b, BLOCK_FLAGS_NONE);
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
					fat_alloc_shadow_clusters(bs,
					    dev_handle, lifo, nclsts);
					*mcl = lifo[found - 1];
					*lcl = lifo[0];
					free(lifo);
					fibril_mutex_unlock(&fat_alloc_lock);
					return EOK;
				}
			}
		}
		block_put(blk);
	}
	fibril_mutex_unlock(&fat_alloc_lock);

	/*
	 * We could not find enough clusters. Now we need to free the clusters
	 * we have allocated so far.
	 */
	while (found--) {
		fat_set_cluster(bs, dev_handle, FAT1, lifo[found],
		    FAT_CLST_RES0);
	}
	
	free(lifo);
	return ENOSPC;
}

/** Free clusters forming a cluster chain in all copies of FAT.
 *
 * @param bs		Buffer hodling the boot sector of the file system.
 * @param dev_handle	Device handle of the file system.
 * @param firstc	First cluster in the chain which is to be freed.
 */
void
fat_free_clusters(fat_bs_t *bs, dev_handle_t dev_handle, fat_cluster_t firstc)
{
	unsigned fatno;
	fat_cluster_t nextc;

	/* Mark all clusters in the chain as free in all copies of FAT. */
	while (firstc < FAT_CLST_LAST1) {
		assert(firstc >= FAT_CLST_FIRST && firstc < FAT_CLST_BAD);
		nextc = fat_get_cluster(bs, dev_handle, firstc);
		for (fatno = FAT1; fatno < bs->fatcnt; fatno++)
			fat_set_cluster(bs, dev_handle, fatno, firstc,
			    FAT_CLST_RES0);
		firstc = nextc;
	}
}

/** Append a cluster chain to the last file cluster in all FATs.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		Node representing the file.
 * @param mcl		First cluster of the cluster chain to append.
 */
void fat_append_clusters(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t mcl)
{
	dev_handle_t dev_handle = nodep->idx->dev_handle;
	fat_cluster_t lcl;
	uint8_t fatno;

	if (fat_cluster_walk(bs, dev_handle, nodep->firstc, &lcl,
	    (uint16_t) -1) == 0) {
		/* No clusters allocated to the node yet. */
		nodep->firstc = mcl;
		nodep->dirty = true;		/* need to sync node */
		return;
	}

	for (fatno = FAT1; fatno < bs->fatcnt; fatno++)
		fat_set_cluster(bs, nodep->idx->dev_handle, fatno, lcl, mcl);
}

/** Chop off node clusters in all copies of FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		FAT node where the chopping will take place.
 * @param lastc		Last cluster which will remain in the node. If this
 *			argument is FAT_CLST_RES0, then all clusters will
 *			be chopped off.
 */
void fat_chop_clusters(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t lastc)
{
	dev_handle_t dev_handle = nodep->idx->dev_handle;
	if (lastc == FAT_CLST_RES0) {
		/* The node will have zero size and no clusters allocated. */
		fat_free_clusters(bs, dev_handle, nodep->firstc);
		nodep->firstc = FAT_CLST_RES0;
		nodep->dirty = true;		/* need to sync node */
	} else {
		fat_cluster_t nextc;
		unsigned fatno;

		nextc = fat_get_cluster(bs, dev_handle, lastc);

		/* Terminate the cluster chain in all copies of FAT. */
		for (fatno = FAT1; fatno < bs->fatcnt; fatno++)
			fat_set_cluster(bs, dev_handle, fatno, lastc, FAT_CLST_LAST1);

		/* Free all following clusters. */
		fat_free_clusters(bs, dev_handle, nextc);
	}
}

/**
 * @}
 */ 
