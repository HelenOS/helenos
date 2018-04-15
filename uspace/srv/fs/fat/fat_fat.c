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
 * @file	fat_fat.c
 * @brief	Functions that manipulate the File Allocation Tables.
 */

#include "fat_fat.h"
#include "fat_dentry.h"
#include "fat.h"
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

#define IS_ODD(number)	(number & 0x1)

/**
 * The fat_alloc_lock mutex protects all copies of the File Allocation Table
 * during allocation of clusters. The lock does not have to be held durring
 * deallocation of clusters.
 */
static FIBRIL_MUTEX_INITIALIZE(fat_alloc_lock);

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
fat_cluster_walk(fat_bs_t *bs, service_id_t service_id, fat_cluster_t firstc,
    fat_cluster_t *lastc, uint32_t *numc, uint32_t max_clusters)
{
	uint32_t clusters = 0;
	fat_cluster_t clst = firstc, clst_last1 = FAT_CLST_LAST1(bs);
	fat_cluster_t clst_bad = FAT_CLST_BAD(bs);
	errno_t rc;

	if (firstc == FAT_CLST_RES0) {
		/* No space allocated to the file. */
		if (lastc)
			*lastc = firstc;
		if (numc)
			*numc = 0;
		return EOK;
	}

	while (clst < clst_last1 && clusters < max_clusters) {
		assert(clst >= FAT_CLST_FIRST);
		if (lastc)
			*lastc = clst;	/* remember the last cluster number */

		/* read FAT1 */
		rc = fat_get_cluster(bs, service_id, FAT1, clst, &clst);
		if (rc != EOK)
			return rc;

		assert(clst != clst_bad);
		clusters++;
	}

	if (lastc && clst < clst_last1)
		*lastc = clst;
	if (numc)
		*numc = clusters;

	return EOK;
}

/** Read block from file located on a FAT file system.
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
fat_block_get(block_t **block, struct fat_bs *bs, fat_node_t *nodep,
    aoff64_t bn, int flags)
{
	fat_cluster_t firstc = nodep->firstc;
	fat_cluster_t currc = 0;
	aoff64_t relbn = bn;
	errno_t rc;

	if (!nodep->size)
		return ELIMIT;

	if (!FAT_IS_FAT32(bs) && nodep->firstc == FAT_CLST_ROOT)
		goto fall_through;

	if (((((nodep->size - 1) / BPS(bs)) / SPC(bs)) == bn / SPC(bs)) &&
	    nodep->lastc_cached_valid) {
	    	/*
		 * This is a request to read a block within the last cluster
		 * when fortunately we have the last cluster number cached.
		 */
		return block_get(block, nodep->idx->service_id,
		    CLBN2PBN(bs, nodep->lastc_cached_value, bn), flags);
	}

	if (nodep->currc_cached_valid && bn >= nodep->currc_cached_bn) {
		/*
		 * We can start with the cluster cached by the previous call to
		 * fat_block_get().
		 */
		firstc = nodep->currc_cached_value;
		relbn -= (nodep->currc_cached_bn / SPC(bs)) * SPC(bs);
	}

fall_through:
	rc = _fat_block_get(block, bs, nodep->idx->service_id, firstc,
	    &currc, relbn, flags);
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
 * @param service_id	Service ID handle of the file system.
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
_fat_block_get(block_t **block, fat_bs_t *bs, service_id_t service_id,
    fat_cluster_t fcl, fat_cluster_t *clp, aoff64_t bn, int flags)
{
	uint32_t clusters;
	uint32_t max_clusters;
	fat_cluster_t c = 0;
	errno_t rc;

	/*
	 * This function can only operate on non-zero length files.
	 */
	if (fcl == FAT_CLST_RES0)
		return ELIMIT;

	if (!FAT_IS_FAT32(bs) && fcl == FAT_CLST_ROOT) {
		/* root directory special case */
		assert(bn < RDS(bs));
		rc = block_get(block, service_id,
		    RSCNT(bs) + FATCNT(bs) * SF(bs) + bn, flags);
		return rc;
	}

	max_clusters = bn / SPC(bs);
	rc = fat_cluster_walk(bs, service_id, fcl, &c, &clusters, max_clusters);
	if (rc != EOK)
		return rc;
	assert(clusters == max_clusters);

	rc = block_get(block, service_id, CLBN2PBN(bs, c, bn), flags);

	if (clp)
		*clp = c;

	return rc;
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
 *
 * @return		EOK on success or an error code.
 */
errno_t
fat_fill_gap(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t mcl, aoff64_t pos)
{
	block_t *b;
	aoff64_t o, boundary;
	errno_t rc;

	boundary = ROUND_UP(nodep->size, BPS(bs) * SPC(bs));

	/* zero out already allocated space */
	for (o = nodep->size; o < pos && o < boundary;
	    o = ALIGN_DOWN(o + BPS(bs), BPS(bs))) {
		int flags = (o % BPS(bs) == 0) ?
		    BLOCK_FLAGS_NOREAD : BLOCK_FLAGS_NONE;
		rc = fat_block_get(&b, bs, nodep, o / BPS(bs), flags);
		if (rc != EOK)
			return rc;
		memset(b->data + o % BPS(bs), 0, BPS(bs) - o % BPS(bs));
		b->dirty = true;		/* need to sync node */
		rc = block_put(b);
		if (rc != EOK)
			return rc;
	}

	if (o >= pos)
		return EOK;

	/* zero out the initial part of the new cluster chain */
	for (o = boundary; o < pos; o += BPS(bs)) {
		rc = _fat_block_get(&b, bs, nodep->idx->service_id, mcl,
		    NULL, (o - boundary) / BPS(bs), BLOCK_FLAGS_NOREAD);
		if (rc != EOK)
			return rc;
		memset(b->data, 0, min(BPS(bs), pos - o));
		b->dirty = true;		/* need to sync node */
		rc = block_put(b);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Get cluster from the first FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or an error code.
 */
static errno_t
fat_get_cluster_fat12(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t *value)
{
	block_t *b, *b1;
	uint16_t byte1, byte2;
	aoff64_t offset;
	errno_t rc;

	offset = (clst + clst / 2);
	if (offset / BPS(bs) >= SF(bs))
		return ERANGE;

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	byte1 = ((uint8_t *) b->data)[offset % BPS(bs)];
	/* This cluster access spans a sector boundary. Check only for FAT12 */
	if ((offset % BPS(bs)) + 1 == BPS(bs)) {
		/* Is this the last sector of FAT? */
		if (offset / BPS(bs) < SF(bs)) {
			/* No, read the next sector */
			rc = block_get(&b1, service_id, 1 + RSCNT(bs) +
			    SF(bs) * fatno + offset / BPS(bs),
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				block_put(b);
				return rc;
			}
			/*
			* Combining value with last byte of current sector and
			* first byte of next sector
			*/
			byte2 = ((uint8_t *) b1->data)[0];

			rc = block_put(b1);
			if (rc != EOK) {
				block_put(b);
				return rc;
			}
		} else {
			/* Yes. This is the last sector of FAT */
			block_put(b);
			return ERANGE;
		}
	} else
		byte2 = ((uint8_t *) b->data)[(offset % BPS(bs)) + 1];

	*value = (byte1 | (byte2 << 8));
	if (IS_ODD(clst))
		*value = (*value) >> 4;
	else
		*value = (*value) & FAT12_MASK;

	rc = block_put(b);

	return rc;
}

/** Get cluster from the first FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or an error code.
 */
static errno_t
fat_get_cluster_fat16(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t *value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;

	offset = (clst * FAT16_CLST_SIZE);

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*value = uint16_t_le2host(*(uint16_t *)(b->data + offset % BPS(bs)));

	rc = block_put(b);

	return rc;
}

/** Get cluster from the first FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or an error code.
 */
static errno_t
fat_get_cluster_fat32(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t *value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;

	offset = (clst * FAT32_CLST_SIZE);

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*value = uint32_t_le2host(*(uint32_t *)(b->data + offset % BPS(bs))) &
	    FAT32_MASK;

	rc = block_put(b);

	return rc;
}


/** Get cluster from the first FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param clst		Cluster which to get.
 * @param value		Output argument holding the value of the cluster.
 *
 * @return		EOK or an error code.
 */
errno_t
fat_get_cluster(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t *value)
{
	errno_t rc;

	assert(fatno < FATCNT(bs));

	if (FAT_IS_FAT12(bs))
		rc = fat_get_cluster_fat12(bs, service_id, fatno, clst, value);
	else if (FAT_IS_FAT16(bs))
		rc = fat_get_cluster_fat16(bs, service_id, fatno, clst, value);
	else
		rc = fat_get_cluster_fat32(bs, service_id, fatno, clst, value);

	return rc;
}

/** Set cluster in one instance of FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param fatno		Number of the FAT instance where to make the change.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or an error code.
 */
static errno_t
fat_set_cluster_fat12(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t value)
{
	block_t *b, *b1 = NULL;
	aoff64_t offset;
	uint16_t byte1, byte2;
	errno_t rc;

	offset = (clst + clst / 2);
	if (offset / BPS(bs) >= SF(bs))
		return ERANGE;

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	byte1 = ((uint8_t *) b->data)[offset % BPS(bs)];
	bool border = false;
	/* This cluster access spans a sector boundary. */
	if ((offset % BPS(bs)) + 1 == BPS(bs)) {
		/* Is it the last sector of FAT? */
		if (offset / BPS(bs) < SF(bs)) {
			/* No, read the next sector */
			rc = block_get(&b1, service_id, 1 + RSCNT(bs) +
			    SF(bs) * fatno + offset / BPS(bs),
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				block_put(b);
				return rc;
			}
			/*
			 * Combining value with last byte of current sector and
			 * first byte of next sector
			 */
			byte2 = ((uint8_t *) b1->data)[0];
			border = true;
		} else {
			/* Yes. This is the last sector of FAT */
			block_put(b);
			return ERANGE;
		}
	} else
		byte2 = ((uint8_t *) b->data)[(offset % BPS(bs)) + 1];

	if (IS_ODD(clst)) {
		byte1 &= 0x0f;
		byte2 = 0;
		value = (value << 4);
	} else {
		byte1 = 0;
		byte2 &= 0xf0;
		value &= FAT12_MASK;
	}

	byte1 = byte1 | (value & 0xff);
	byte2 = byte2 | (value >> 8);

	((uint8_t *) b->data)[(offset % BPS(bs))] = byte1;
	if (border) {
		((uint8_t *) b1->data)[0] = byte2;

		b1->dirty = true;
		rc = block_put(b1);
		if (rc != EOK) {
			block_put(b);
			return rc;
		}
	} else
		((uint8_t *) b->data)[(offset % BPS(bs)) + 1] = byte2;

	b->dirty = true;	/* need to sync block */
	rc = block_put(b);

	return rc;
}

/** Set cluster in one instance of FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param fatno		Number of the FAT instance where to make the change.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or an error code.
 */
static errno_t
fat_set_cluster_fat16(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;

	offset = (clst * FAT16_CLST_SIZE);

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	*(uint16_t *)(b->data + offset % BPS(bs)) = host2uint16_t_le(value);

	b->dirty = true;	/* need to sync block */
	rc = block_put(b);

	return rc;
}

/** Set cluster in one instance of FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Service ID for the file system.
 * @param fatno		Number of the FAT instance where to make the change.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or an error code.
 */
static errno_t
fat_set_cluster_fat32(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t value)
{
	block_t *b;
	aoff64_t offset;
	errno_t rc;
	fat_cluster_t temp;

	offset = (clst * FAT32_CLST_SIZE);

	rc = block_get(&b, service_id, RSCNT(bs) + SF(bs) * fatno +
	    offset / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	temp = uint32_t_le2host(*(uint32_t *)(b->data + offset % BPS(bs)));
	temp &= 0xf0000000;
	temp |= (value & FAT32_MASK);
	*(uint32_t *)(b->data + offset % BPS(bs)) = host2uint32_t_le(temp);

	b->dirty = true;	/* need to sync block */
	rc = block_put(b);

	return rc;
}

/** Set cluster in one instance of FAT.
 *
 * @param bs		Buffer holding the boot sector for the file system.
 * @param service_id	Device service ID for the file system.
 * @param fatno		Number of the FAT instance where to make the change.
 * @param clst		Cluster which is to be set.
 * @param value		Value to set the cluster with.
 *
 * @return		EOK on success or an error code.
 */
errno_t
fat_set_cluster(fat_bs_t *bs, service_id_t service_id, unsigned fatno,
    fat_cluster_t clst, fat_cluster_t value)
{
	errno_t rc;

	assert(fatno < FATCNT(bs));

	if (FAT_IS_FAT12(bs))
		rc = fat_set_cluster_fat12(bs, service_id, fatno, clst, value);
	else if (FAT_IS_FAT16(bs))
		rc = fat_set_cluster_fat16(bs, service_id, fatno, clst, value);
	else
		rc = fat_set_cluster_fat32(bs, service_id, fatno, clst, value);

	return rc;
}

/** Replay the allocatoin of clusters in all shadow instances of FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param service_id	Service ID of the file system.
 * @param lifo		Chain of allocated clusters.
 * @param nclsts	Number of clusters in the lifo chain.
 *
 * @return		EOK on success or an error code.
 */
errno_t fat_alloc_shadow_clusters(fat_bs_t *bs, service_id_t service_id,
    fat_cluster_t *lifo, unsigned nclsts)
{
	uint8_t fatno;
	unsigned c;
	fat_cluster_t clst_last1 = FAT_CLST_LAST1(bs);
	errno_t rc;

	for (fatno = FAT1 + 1; fatno < FATCNT(bs); fatno++) {
		for (c = 0; c < nclsts; c++) {
			rc = fat_set_cluster(bs, service_id, fatno, lifo[c],
			    c == 0 ? clst_last1 : lifo[c - 1]);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

/** Allocate clusters in all copies of FAT.
 *
 * This function will attempt to allocate the requested number of clusters in
 * all instances of the FAT.  The FAT will be altered so that the allocated
 * clusters form an independent chain (i.e. a chain which does not belong to any
 * file yet).
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param service_id	Device service ID of the file system.
 * @param nclsts	Number of clusters to allocate.
 * @param mcl		Output parameter where the first cluster in the chain
 *			will be returned.
 * @param lcl		Output parameter where the last cluster in the chain
 *			will be returned.
 *
 * @return		EOK on success, an error code otherwise.
 */
errno_t
fat_alloc_clusters(fat_bs_t *bs, service_id_t service_id, unsigned nclsts,
    fat_cluster_t *mcl, fat_cluster_t *lcl)
{
	fat_cluster_t *lifo;    /* stack for storing free cluster numbers */
	unsigned found = 0;     /* top of the free cluster number stack */
	fat_cluster_t clst;
	fat_cluster_t value = 0;
	fat_cluster_t clst_last1 = FAT_CLST_LAST1(bs);
	errno_t rc = EOK;

	lifo = (fat_cluster_t *) malloc(nclsts * sizeof(fat_cluster_t));
	if (!lifo)
		return ENOMEM;

	/*
	 * Search FAT1 for unused clusters.
	 */
	fibril_mutex_lock(&fat_alloc_lock);
	for (clst = FAT_CLST_FIRST; clst < CC(bs) + 2 && found < nclsts;
	    clst++) {
		rc = fat_get_cluster(bs, service_id, FAT1, clst, &value);
		if (rc != EOK)
			break;

		if (value == FAT_CLST_RES0) {
			/*
			 * The cluster is free. Put it into our stack
			 * of found clusters and mark it as non-free.
			 */
			lifo[found] = clst;
			rc = fat_set_cluster(bs, service_id, FAT1, clst,
			    (found == 0) ?  clst_last1 : lifo[found - 1]);
			if (rc != EOK)
				break;

			found++;
		}
	}

	if (rc == EOK && found == nclsts) {
		rc = fat_alloc_shadow_clusters(bs, service_id, lifo, nclsts);
		if (rc == EOK) {
			*mcl = lifo[found - 1];
			*lcl = lifo[0];
			free(lifo);
			fibril_mutex_unlock(&fat_alloc_lock);
			return EOK;
		}
	}

	/* If something wrong - free the clusters */
	while (found--) {
		(void) fat_set_cluster(bs, service_id, FAT1, lifo[found],
		    FAT_CLST_RES0);
	}

	free(lifo);
	fibril_mutex_unlock(&fat_alloc_lock);

	return ENOSPC;
}

/** Free clusters forming a cluster chain in all copies of FAT.
 *
 * @param bs		Buffer hodling the boot sector of the file system.
 * @param service_id	Device service ID of the file system.
 * @param firstc	First cluster in the chain which is to be freed.
 *
 * @return		EOK on success or an error code.
 */
errno_t
fat_free_clusters(fat_bs_t *bs, service_id_t service_id, fat_cluster_t firstc)
{
	unsigned fatno;
	fat_cluster_t nextc = 0;
	fat_cluster_t clst_bad = FAT_CLST_BAD(bs);
	errno_t rc;

	/* Mark all clusters in the chain as free in all copies of FAT. */
	while (firstc < FAT_CLST_LAST1(bs)) {
		assert(firstc >= FAT_CLST_FIRST && firstc < clst_bad);

		rc = fat_get_cluster(bs, service_id, FAT1, firstc, &nextc);
		if (rc != EOK)
			return rc;

		for (fatno = FAT1; fatno < FATCNT(bs); fatno++) {
			rc = fat_set_cluster(bs, service_id, fatno, firstc,
			    FAT_CLST_RES0);
			if (rc != EOK)
				return rc;
		}

		firstc = nextc;
	}

	return EOK;
}

/** Append a cluster chain to the last file cluster in all FATs.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		Node representing the file.
 * @param mcl		First cluster of the cluster chain to append.
 * @param lcl		Last cluster of the cluster chain to append.
 *
 * @return		EOK on success or an error code.
 */
errno_t fat_append_clusters(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t mcl,
    fat_cluster_t lcl)
{
	service_id_t service_id = nodep->idx->service_id;
	fat_cluster_t lastc = 0;
	uint8_t fatno;
	errno_t rc;

	if (nodep->firstc == FAT_CLST_RES0) {
		/* No clusters allocated to the node yet. */
		nodep->firstc = mcl;
		nodep->dirty = true;	/* need to sync node */
	} else {
		if (nodep->lastc_cached_valid) {
			lastc = nodep->lastc_cached_value;
			nodep->lastc_cached_valid = false;
		} else {
			rc = fat_cluster_walk(bs, service_id, nodep->firstc,
			    &lastc, NULL, (uint32_t) -1);
			if (rc != EOK)
				return rc;
		}

		for (fatno = FAT1; fatno < FATCNT(bs); fatno++) {
			rc = fat_set_cluster(bs, nodep->idx->service_id,
			    fatno, lastc, mcl);
			if (rc != EOK)
				return rc;
		}
	}

	nodep->lastc_cached_valid = true;
	nodep->lastc_cached_value = lcl;

	return EOK;
}

/** Chop off node clusters in all copies of FAT.
 *
 * @param bs		Buffer holding the boot sector of the file system.
 * @param nodep		FAT node where the chopping will take place.
 * @param lcl		Last cluster which will remain in the node. If this
 *			argument is FAT_CLST_RES0, then all clusters will
 *			be chopped off.
 *
 * @return		EOK on success or an error code.
 */
errno_t fat_chop_clusters(fat_bs_t *bs, fat_node_t *nodep, fat_cluster_t lcl)
{
	fat_cluster_t clst_last1 = FAT_CLST_LAST1(bs);
	errno_t rc;
	service_id_t service_id = nodep->idx->service_id;

	/*
	 * Invalidate cached cluster numbers.
	 */
	nodep->lastc_cached_valid = false;
	if (nodep->currc_cached_value != lcl)
		nodep->currc_cached_valid = false;

	if (lcl == FAT_CLST_RES0) {
		/* The node will have zero size and no clusters allocated. */
		rc = fat_free_clusters(bs, service_id, nodep->firstc);
		if (rc != EOK)
			return rc;
		nodep->firstc = FAT_CLST_RES0;
		nodep->dirty = true;		/* need to sync node */
	} else {
		fat_cluster_t nextc;
		unsigned fatno;

		rc = fat_get_cluster(bs, service_id, FAT1, lcl, &nextc);
		if (rc != EOK)
			return rc;

		/* Terminate the cluster chain in all copies of FAT. */
		for (fatno = FAT1; fatno < FATCNT(bs); fatno++) {
			rc = fat_set_cluster(bs, service_id, fatno, lcl,
			    clst_last1);
			if (rc != EOK)
				return rc;
		}

		/* Free all following clusters. */
		rc = fat_free_clusters(bs, service_id, nextc);
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
fat_zero_cluster(struct fat_bs *bs, service_id_t service_id, fat_cluster_t c)
{
	int i;
	block_t *b;
	errno_t rc;

	for (i = 0; i < SPC(bs); i++) {
		rc = _fat_block_get(&b, bs, service_id, c, NULL, i,
		    BLOCK_FLAGS_NOREAD);
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

/** Perform basic sanity checks on the file system.
 *
 * Verify if values of boot sector fields are sane. Also verify media
 * descriptor. This is used to rule out cases when a device obviously
 * does not contain a fat file system.
 */
errno_t fat_sanity_check(fat_bs_t *bs, service_id_t service_id)
{
	fat_cluster_t e0 = 0;
	fat_cluster_t e1 = 0;
	unsigned fat_no;
	errno_t rc;

	/* Check number of FATs. */
	if (FATCNT(bs) == 0)
		return ENOTSUP;

	/* Check total number of sectors. */
	if (TS(bs) == 0)
		return ENOTSUP;

	if (bs->totsec16 != 0 && bs->totsec32 != 0 &&
	    bs->totsec16 != bs->totsec32)
		return ENOTSUP;

	/* Check media descriptor. Must be between 0xf0 and 0xff. */
	if ((bs->mdesc & 0xf0) != 0xf0)
		return ENOTSUP;

	/* Check number of sectors per FAT. */
	if (SF(bs) == 0)
		return ENOTSUP;

	/*
	 * Check that the root directory entries take up whole blocks.
	 * This check is rather strict, but it allows us to treat the root
	 * directory and non-root directories uniformly in some places.
	 * It can be removed provided that functions such as fat_read() are
	 * sanitized to support file systems with this property.
	 */
	if (!FAT_IS_FAT32(bs) &&
	    (RDE(bs) * sizeof(fat_dentry_t)) % BPS(bs) != 0)
		return ENOTSUP;

	/* Check signature of each FAT. */
	for (fat_no = 0; fat_no < FATCNT(bs); fat_no++) {
		rc = fat_get_cluster(bs, service_id, fat_no, 0, &e0);
		if (rc != EOK)
			return EIO;

		rc = fat_get_cluster(bs, service_id, fat_no, 1, &e1);
		if (rc != EOK)
			return EIO;

		/*
		 * Check that first byte of FAT contains the media descriptor.
		 */
		if ((e0 & 0xff) != bs->mdesc)
			return ENOTSUP;

		/*
		 * Check that remaining bits of the first two entries are
		 * set to one.
		 */
		if (!FAT_IS_FAT12(bs) &&
		    ((e0 >> 8) != (FAT_MASK(bs) >> 8) || e1 != FAT_MASK(bs)))
			return ENOTSUP;
	}

	return EOK;
}

/**
 * @}
 */
