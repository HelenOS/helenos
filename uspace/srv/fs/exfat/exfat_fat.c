/*
 * Copyright (c) 2008 Jakub Jermar
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
 * @file	exfat_fat.c
 * @brief	Functions that manipulate the File Allocation Table.
 */

#include "exfat_fat.h"
#include "exfat_bitmap.h"
#include "exfat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <block.h>
#include <errno.h>
#include <byteorder.h>
#include <align.h>
#include <assert.h>
#include <fibril_synch.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>


/**
 * The fat_alloc_lock mutex protects all copies of the File Allocation Table
 * during allocation of clusters. The lock does not have to be held durring
 * deallocation of clusters.
 */
static FIBRIL_MUTEX_INITIALIZE(exfat_alloc_lock);

/** Walk the cluster chain.
 *
 * @param bs		Buffer holding the boot sector for the file.
 * @param service_id	Service ID of the device with the file.
 * @param firstc	First cluster to start the walk with.
 * @param lastc		If non-NULL, output argument hodling the last cluster
 *			number visited.
 * @param numc		If non-NULL, output argument holding the number of
 *			clusters seen during the walk.
 * @param max_clusters	Maximum number of clusters to visit.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_cluster_walk(exfat_bs_t *bs, service_id_t service_id, 
    exfat_cluster_t firstc, exfat_cluster_t *lastc, uint32_t *numc,
    uint32_t max_clusters)
{
	uint32_t clusters = 0;
	exfat_cluster_t clst = firstc;
	errno_t rc;

	if (firstc < EXFAT_CLST_FIRST) {
		/* No space allocated to the file. */
		if (lastc)
			*lastc = firstc;
		if (numc)
			*numc = 0;
		return EOK;
	}

	while (clst != EXFAT_CLST_EOF && clusters < max_clusters) {
		assert(clst >= EXFAT_CLST_FIRST);
		if (lastc)
			*lastc = clst;	/* remember the last cluster number */

		rc = exfat_get_cluster(bs, service_id, clst, &clst);
		if (rc != EOK)
			return rc;

		assert(clst != EXFAT_CLST_BAD);
		clusters++;
	}

	if (lastc && clst != EXFAT_CLST_EOF)
		*lastc = clst;
	if (numc)
		*numc = clusters;

	return EOK;
}

/** Read block from file located on a exFAT file system.
 *
 * @param block		Pointer to a block pointer for storing result.
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		FAT node.
 * @param bn		Block number.
 * @param flags		Flags passed to libblock.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_block_get(block_t **block, exfat_bs_t *bs, exfat_node_t *nodep,
    aoff64_t bn, int flags)
{
	exfat_cluster_t firstc = nodep->firstc;
	exfat_cluster_t currc = 0;
	aoff64_t relbn = bn;
	errno_t rc;

	if (!nodep->size)
		return ELIMIT;

	if (nodep->fragmented) {
		if (((((nodep->size - 1) / BPS(bs)) / SPC(bs)) == bn / SPC(bs)) &&
		    nodep->lastc_cached_valid) {
			/*
			 * This is a request to read a block within the last cluster
			 * when fortunately we have the last cluster number cached.
			 */
			return block_get(block, nodep->idx->service_id, DATA_FS(bs) + 
		        (nodep->lastc_cached_value-EXFAT_CLST_FIRST)*SPC(bs) + 
			    (bn % SPC(bs)), flags);
		}

		if (nodep->currc_cached_valid && bn >= nodep->currc_cached_bn) {
			/*
			* We can start with the cluster cached by the previous call to
			* fat_block_get().
			*/
			firstc = nodep->currc_cached_value;
			relbn -= (nodep->currc_cached_bn / SPC(bs)) * SPC(bs);
		}
	}

	rc = exfat_block_get_by_clst(block, bs, nodep->idx->service_id,
	    nodep->fragmented, firstc, &currc, relbn, flags);
	if (rc != EOK)
		return rc;

	/*
	 * Update the "current" cluster cache.
	 */
	nodep->currc_cached_valid = true;
	nodep->currc_cached_bn = bn;
	nodep->currc_cached_value = currc;

	return rc;
}

/** Read block from file located on a exFAT file system.
 *
 * @param block		Pointer to a block pointer for storing result.
 * @param bs		Buffer holding the boot sector of the file system.
 * @param service_id	Service ID of the file system.
 * @param fcl		First cluster used by the file. Can be zero if the file
 *			is empty.
 * @param clp		If not NULL, address where the cluster containing bn
 *			will be stored.
 *			stored
 * @param bn		Block number.
 * @param flags		Flags passed to libblock.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_block_get_by_clst(block_t **block, exfat_bs_t *bs, 
    service_id_t service_id, bool fragmented, exfat_cluster_t fcl,
    exfat_cluster_t *clp, aoff64_t bn, int flags)
{
	uint32_t clusters;
	uint32_t max_clusters;
	exfat_cluster_t c = EXFAT_CLST_FIRST;
	errno_t rc;

	if (fcl < EXFAT_CLST_FIRST || fcl > DATA_CNT(bs) + 2)
		return ELIMIT;

	if (!fragmented) {
		rc = block_get(block, service_id, DATA_FS(bs) + 
		    (fcl - EXFAT_CLST_FIRST)*SPC(bs) + bn, flags);
	} else {
		max_clusters = bn / SPC(bs);
		rc = exfat_cluster_walk(bs, service_id, fcl, &c, &clusters, max_clusters);
		if (rc != EOK)
			return rc;
		assert(clusters == max_clusters);

		rc = block_get(block, service_id, DATA_FS(bs) + 
		    (c - EXFAT_CLST_FIRST) * SPC(bs) + (bn % SPC(bs)), flags);

		if (clp)
			*clp = c;
	}

	return rc;
}


/** Get cluster from the FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or an error code.
 */
errno_t
exfat_get_cluster(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t clst, exfat_cluster_t *value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;

	offset = clst * sizeof(exfat_cluster_t);

	rc = block_get(&b, service_id, FAT_FS(bs) + offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*value = uint32_t_le2host(*(uint32_t *)(b->data + offset % BPS(bs)));

	rc = block_put(b);

	return rc;
}

/** Set cluster in FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_set_cluster(exfat_bs_t *bs, service_id_t service_id,
    exfat_cluster_t clst, exfat_cluster_t value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;

	offset = clst * sizeof(exfat_cluster_t);

	rc = block_get(&b, service_id, FAT_FS(bs) + offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*(uint32_t *)(b->data + offset % BPS(bs)) = host2uint32_t_le(value);

	b->dirty = true;	/* need to sync block */
	rc = block_put(b);
	return rc;
}

/** Allocate clusters in FAT.
 *
 * This function will attempt to allocate the requested number of clusters in
 * the FAT.  The FAT will be altered so that the allocated
 * clusters form an independent chain (i.e. a chain which does not belong to any
 * file yet).
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param service_id	Service ID of the file system.
 * @param nclsts	Number of clusters to allocate.
 * @param mcl		Output parameter where the first cluster in the chain
 *			will be returned.
 * @param lcl		Output parameter where the last cluster in the chain
 *			will be returned.
 *
 * @return		EOK on success, an error code otherwise.
 */
errno_t
exfat_alloc_clusters(exfat_bs_t *bs, service_id_t service_id, unsigned nclsts,
    exfat_cluster_t *mcl, exfat_cluster_t *lcl)
{
	exfat_cluster_t *lifo;    /* stack for storing free cluster numbers */
	unsigned found = 0;     /* top of the free cluster number stack */
	exfat_cluster_t clst;
	errno_t rc = EOK;

	lifo = (exfat_cluster_t *) malloc(nclsts * sizeof(exfat_cluster_t));
	if (!lifo)
		return ENOMEM;

	fibril_mutex_lock(&exfat_alloc_lock);
	for (clst = EXFAT_CLST_FIRST; clst < DATA_CNT(bs) + 2 && found < nclsts;
	    clst++) {
		/* Need to rewrite because of multiple exfat_bitmap_get calls */
		if (exfat_bitmap_is_free(bs, service_id, clst) == EOK) {
			/*
			 * The cluster is free. Put it into our stack
			 * of found clusters and mark it as non-free.
			 */
			lifo[found] = clst;
			rc = exfat_set_cluster(bs, service_id, clst,
			    (found == 0) ?  EXFAT_CLST_EOF : lifo[found - 1]);
			if (rc != EOK)
				goto exit_error;
			found++;
			rc = exfat_bitmap_set_cluster(bs, service_id, clst);
			if (rc != EOK)
				goto exit_error;

		}
	}

	if (rc == EOK && found == nclsts) {
		*mcl = lifo[found - 1];
		*lcl = lifo[0];
		free(lifo);
		fibril_mutex_unlock(&exfat_alloc_lock);
		return EOK;
	}

	rc = ENOSPC;

exit_error:

	/* If something wrong - free the clusters */
	while (found--) {
		(void) exfat_bitmap_clear_cluster(bs, service_id, lifo[found]);
		(void) exfat_set_cluster(bs, service_id, lifo[found], 0);
	}

	free(lifo);
	fibril_mutex_unlock(&exfat_alloc_lock);
	return rc;
}

/** Free clusters forming a cluster chain in FAT.
 *
 * @param bs		Buffer hodling the boot sector of the file system.
 * @param service_id	Service ID of the file system.
 * @param firstc	First cluster in the chain which is to be freed.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_free_clusters(exfat_bs_t *bs, service_id_t service_id, exfat_cluster_t firstc)
{
	exfat_cluster_t nextc;
	errno_t rc;

	/* Mark all clusters in the chain as free */
	while (firstc != EXFAT_CLST_EOF) {
		assert(firstc >= EXFAT_CLST_FIRST && firstc < EXFAT_CLST_BAD);
		rc = exfat_get_cluster(bs, service_id, firstc, &nextc);
		if (rc != EOK)
			return rc;
		rc = exfat_set_cluster(bs, service_id, firstc, 0);
		if (rc != EOK)
			return rc;
		rc = exfat_bitmap_clear_cluster(bs, service_id, firstc);
		if (rc != EOK)
			return rc;
		firstc = nextc;
	}

	return EOK;
}

/** Append a cluster chain to the last file cluster in FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		Node representing the file.
 * @param mcl		First cluster of the cluster chain to append.
 * @param lcl		Last cluster of the cluster chain to append.
 *
 * @return		EOK on success or an error code.
 */
errno_t
exfat_append_clusters(exfat_bs_t *bs, exfat_node_t *nodep, exfat_cluster_t mcl,
    exfat_cluster_t lcl)
{
	service_id_t service_id = nodep->idx->service_id;
	exfat_cluster_t lastc = 0;
	errno_t rc;

	if (nodep->firstc == 0) {
		/* No clusters allocated to the node yet. */
		nodep->firstc = mcl;
		nodep->dirty = true;	/* need to sync node */
	} else {
		if (nodep->lastc_cached_valid) {
			lastc = nodep->lastc_cached_value;
			nodep->lastc_cached_valid = false;
		} else {
			rc = exfat_cluster_walk(bs, service_id, nodep->firstc,
			    &lastc, NULL, (uint16_t) -1);
			if (rc != EOK)
				return rc;
		}

		rc = exfat_set_cluster(bs, nodep->idx->service_id, lastc, mcl);
		if (rc != EOK)
			return rc;
	}

	nodep->lastc_cached_valid = true;
	nodep->lastc_cached_value = lcl;

	return EOK;
}

/** Chop off node clusters in FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		FAT node where the chopping will take place.
 * @param lcl		Last cluster which will remain in the node. If this
 *			argument is FAT_CLST_RES0, then all clusters will
 *			be chopped off.
 *
 * @return		EOK on success or an error code.
 */
errno_t exfat_chop_clusters(exfat_bs_t *bs, exfat_node_t *nodep, exfat_cluster_t lcl)
{
	errno_t rc;
	service_id_t service_id = nodep->idx->service_id;

	/*
	 * Invalidate cached cluster numbers.
	 */
	nodep->lastc_cached_valid = false;
	if (nodep->currc_cached_value != lcl)
		nodep->currc_cached_valid = false;

	if (lcl == 0) {
		/* The node will have zero size and no clusters allocated. */
		rc = exfat_free_clusters(bs, service_id, nodep->firstc);
		if (rc != EOK)
			return rc;
		nodep->firstc = 0;
		nodep->dirty = true;		/* need to sync node */
	} else {
		exfat_cluster_t nextc;

		rc = exfat_get_cluster(bs, service_id, lcl, &nextc);
		if (rc != EOK)
			return rc;

		/* Terminate the cluster chain */
		rc = exfat_set_cluster(bs, service_id, lcl, EXFAT_CLST_EOF);
		if (rc != EOK)
			return rc;

		/* Free all following clusters. */
		rc = exfat_free_clusters(bs, service_id, nextc);
		if (rc != EOK)
			return rc;
	}

	/*
	 * Update and re-enable the last cluster cache.
	 */
	nodep->lastc_cached_valid = true;
	nodep->lastc_cached_value = lcl;

	return EOK;
}

errno_t
exfat_zero_cluster(exfat_bs_t *bs, service_id_t service_id, exfat_cluster_t c)
{
	size_t i;
	block_t *b;
	errno_t rc;

	for (i = 0; i < SPC(bs); i++) {
		rc = exfat_block_get_by_clst(&b, bs, service_id, false, c, NULL,
		    i, BLOCK_FLAGS_NOREAD);
		if (rc != EOK)
			return rc;
		memset(b->data, 0, BPS(bs));
		b->dirty = true;
		rc = block_put(b);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

errno_t
exfat_read_uctable(exfat_bs_t *bs, exfat_node_t *nodep, uint8_t *uctable)
{
	size_t i, blocks, count;
	block_t *b;
	errno_t rc;
	blocks = ROUND_UP(nodep->size, BPS(bs))/BPS(bs);
	count = BPS(bs);
	
	for (i = 0; i < blocks; i++) {
		rc = exfat_block_get(&b, bs, nodep, i, BLOCK_FLAGS_NOREAD);
		if (rc != EOK)
			return rc;
		if (i == blocks - 1)
			count = nodep->size - i * BPS(bs);
		memcpy(uctable, b->data, count);
		uctable += count;
		rc = block_put(b);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}


/** Perform basic sanity checks on the file system.
 *
 * Verify if values of boot sector fields are sane. Also verify media
 * descriptor. This is used to rule out cases when a device obviously
 * does not contain a exfat file system.
 */
errno_t exfat_sanity_check(exfat_bs_t *bs)
{
	if (str_cmp((char const *)bs->oem_name, "EXFAT   "))
		return ENOTSUP;
	else if (uint16_t_le2host(bs->signature) != 0xAA55)
		return ENOTSUP;
	else if (uint32_t_le2host(bs->fat_sector_count) == 0)
		return ENOTSUP;
	else if (uint32_t_le2host(bs->data_clusters) == 0)
		return ENOTSUP;
	else if (bs->fat_count != 1)
		return ENOTSUP;
	else if ((bs->bytes_per_sector + bs->sec_per_cluster) > 25) {
		/* exFAT does not support cluster size > 32 Mb */
		return ENOTSUP;
	}
	return EOK;
}

/**
 * @}
 */
