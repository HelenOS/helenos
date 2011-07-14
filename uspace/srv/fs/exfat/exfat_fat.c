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
