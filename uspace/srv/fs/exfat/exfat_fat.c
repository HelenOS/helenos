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
 * @brief	Functions that manipulate the File Allocation Tables.
 */

#include "exfat_fat.h"
#include "exfat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <errno.h>
#include <byteorder.h>
#include <align.h>
#include <assert.h>
#include <fibril_synch.h>
#include <malloc.h>
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
 * @param devmap_handle	Device handle of the device with the file.
 * @param firstc	First cluster to start the walk with.
 * @param lastc		If non-NULL, output argument hodling the last cluster
 *			number visited.
 * @param numc		If non-NULL, output argument holding the number of
 *			clusters seen during the walk.
 * @param max_clusters	Maximum number of clusters to visit.
 *
 * @return		EOK on success or a negative error code.
 */
int
fat_cluster_walk(exfat_bs_t *bs, devmap_handle_t devmap_handle, 
    exfat_cluster_t firstc, exfat_cluster_t *lastc, uint32_t *numc,
    uint32_t max_clusters)
{
	uint32_t clusters = 0;
	exfat_cluster_t clst = firstc;
	int rc;

	if (firstc < EXFAT_CLST_FIRST) {
		/* No space allocated to the file. */
		if (lastc)
			*lastc = firstc;
		if (numc)
			*numc = 0;
		return EOK;
	}

	while (clst <= EXFAT_CLST_LAST && clusters < max_clusters) {
		assert(clst >= EXFAT_CLST_FIRST);
		if (lastc)
			*lastc = clst;	/* remember the last cluster number */

		rc = fat_get_cluster(bs, devmap_handle, clst, &clst);
		if (rc != EOK)
			return rc;

		assert(clst != EXFAT_CLST_BAD);
		clusters++;
	}

	if (lastc && clst <= EXFAT_CLST_LAST)
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
 * @return		EOK on success or a negative error code.
 */
int
exfat_block_get(block_t **block, exfat_bs_t *bs, exfat_node_t *nodep,
    aoff64_t bn, int flags)
{
	exfat_cluster_t firstc = nodep->firstc;
	exfat_cluster_t currc;
	aoff64_t relbn = bn;
	int rc;

	if (!nodep->size)
		return ELIMIT;

	if (nodep->fragmented) {
		if (((((nodep->size - 1) / BPS(bs)) / SPC(bs)) == bn / SPC(bs)) &&
			nodep->lastc_cached_valid) {
				/*
			* This is a request to read a block within the last cluster
			* when fortunately we have the last cluster number cached.
			*/
			return block_get(block, nodep->idx->devmap_handle, DATA_FS(bs) + 
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

	rc = exfat_block_get_by_clst(block, bs, nodep->idx->devmap_handle,
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

/** Read block from file located on a FAT file system.
 *
 * @param block		Pointer to a block pointer for storing result.
 * @param bs		Buffer holding the boot sector of the file system.
 * @param devmap_handle	Device handle of the file system.
 * @param fcl		First cluster used by the file. Can be zero if the file
 *			is empty.
 * @param clp		If not NULL, address where the cluster containing bn
 *			will be stored.
 *			stored
 * @param bn		Block number.
 * @param flags		Flags passed to libblock.
 *
 * @return		EOK on success or a negative error code.
 */
int
exfat_block_get_by_clst(block_t **block, exfat_bs_t *bs, 
    devmap_handle_t devmap_handle, bool fragmented, exfat_cluster_t fcl,
    exfat_cluster_t *clp, aoff64_t bn, int flags)
{
	uint32_t clusters;
	uint32_t max_clusters;
	exfat_cluster_t c;
	int rc;

	if (fcl < EXFAT_CLST_FIRST)
		return ELIMIT;

	if (!fragmented) {
		rc = block_get(block, devmap_handle, DATA_FS(bs) + 
		    (fcl-EXFAT_CLST_FIRST)*SPC(bs) + bn, flags);
	} else {
		max_clusters = bn / SPC(bs);
		rc = fat_cluster_walk(bs, devmap_handle, fcl, &c, &clusters, max_clusters);
		if (rc != EOK)
			return rc;
		assert(clusters == max_clusters);

		rc = block_get(block, devmap_handle, DATA_FS(bs) + 
		    (c-EXFAT_CLST_FIRST)*SPC(bs) + (bn % SPC(bs)), flags);

		if (clp)
			*clp = c;
	}

	return rc;
}


/** Get cluster from the FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param devmap_handle	Device handle for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or a negative error code.
 */
int
fat_get_cluster(exfat_bs_t *bs, devmap_handle_t devmap_handle,
    exfat_cluster_t clst, exfat_cluster_t *value)
{
	block_t *b;
	aoff64_t offset;
	int rc;

	offset = clst * sizeof(exfat_cluster_t);

	rc = block_get(&b, devmap_handle, FAT_FS(bs) + offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*value = uint32_t_le2host(*(uint32_t *)(b->data + offset % BPS(bs)));

	rc = block_put(b);

	return rc;
}

/** Set cluster in FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param devmap_handle	Device handle for the file system.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or a negative error code.
 */
int
fat_set_cluster(exfat_bs_t *bs, devmap_handle_t devmap_handle,
    exfat_cluster_t clst, exfat_cluster_t value)
{
	block_t *b;
	aoff64_t offset;
	int rc;

	offset = clst * sizeof(exfat_cluster_t);

	rc = block_get(&b, devmap_handle, FAT_FS(bs) + offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*(uint32_t *)(b->data + offset % BPS(bs)) = host2uint32_t_le(value);

	b->dirty = true;	/* need to sync block */
	rc = block_put(b);
	return rc;
}

/** Perform basic sanity checks on the file system.
 *
 * Verify if values of boot sector fields are sane. Also verify media
 * descriptor. This is used to rule out cases when a device obviously
 * does not contain a exfat file system.
 */
int exfat_sanity_check(exfat_bs_t *bs, devmap_handle_t devmap_handle)
{
	return EOK;
}

/**
 * @}
 */
