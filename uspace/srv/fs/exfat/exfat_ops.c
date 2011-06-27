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
 * @file	exfat_ops.c
 * @brief	Implementation of VFS operations for the exFAT file system server.
 */

#include "exfat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <macros.h>
#include <async.h>
#include <errno.h>
#include <str.h>
#include <byteorder.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <sys/mman.h>
#include <align.h>
#include <malloc.h>

/** libfs operations */

libfs_ops_t exfat_libfs_ops;
/*
libfs_ops_t exfat_libfs_ops = {
	.root_get = exfat_root_get,
	.match = exfat_match,
	.node_get = exfat_node_get,
	.node_open = exfat_node_open,
	.node_put = exfat_node_put,
	.create = exfat_create_node,
	.destroy = exfat_destroy_node,
	.link = exfat_link,
	.unlink = exfat_unlink,
	.has_children = exfat_has_children,
	.index_get = exfat_index_get,
	.size_get = exfat_size_get,
	.lnkcnt_get = exfat_lnkcnt_get,
	.plb_get_char = exfat_plb_get_char,
	.is_directory = exfat_is_directory,
	.is_file = exfat_is_file,
	.device_get = exfat_device_get
};
*/

/*
 * VFS operations.
 */

void exfat_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	enum cache_mode cmode;
	exfat_bs_t *bs;

	/* Accept the mount options */
	char *opts;
	int rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);

	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	free(opts);

	/* initialize libblock */
	rc = block_init(devmap_handle, BS_SIZE);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* prepare the boot block */
	rc = block_bb_read(devmap_handle, BS_BLOCK);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	/* get the buffer with the boot sector */
	bs = block_bb_get(devmap_handle);
	
	(void) bs;

	/* Initialize the block cache */
	rc = block_cache_init(devmap_handle, BS_SIZE, 0 /* XXX */, cmode);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	async_answer_0(rid, EOK);
/*	async_answer_3(rid, EOK, ridxp->index, rootp->size, rootp->lnkcnt); */
}

void exfat_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&exfat_libfs_ops, exfat_reg.fs_handle, rid, request);
}

void exfat_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);

	/*
	 * Perform cleanup of the node structures, index structures and
	 * associated data. Write back this file system's dirty blocks and
	 * stop using libblock for this instance.
	 */
	(void) block_cache_fini(devmap_handle);
	block_fini(devmap_handle);

	async_answer_0(rid, EOK);
}

void exfat_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&exfat_libfs_ops, rid, request);
}


/**
 * @}
 */
