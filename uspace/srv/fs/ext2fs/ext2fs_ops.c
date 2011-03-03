/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Martin Sucha
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
 * @file	ext2fs_ops.c
 * @brief	Implementation of VFS operations for the EXT2 file system server.
 */

#include "ext2fs.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <libext2.h>
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
#include <adt/hash_table.h>

#define EXT2_NODE(node)	((node) ? (ext2fs_node_t *) (node)->data : NULL)
#define FS_NODE(node)	((node) ? (node)->bp : NULL)

#define FS_BUCKETS_LOG	12
#define FS_BUCKETS	(1 << FS_BUCKETS_LOG)

/*
 * Forward declarations of EXT2 libfs operations.
 */
static int ext2fs_root_get(fs_node_t **, devmap_handle_t);
static int ext2fs_match(fs_node_t **, fs_node_t *, const char *);
static int ext2fs_node_get(fs_node_t **, devmap_handle_t, fs_index_t);
static int ext2fs_node_open(fs_node_t *);
static int ext2fs_node_put(fs_node_t *);
static int ext2fs_create_node(fs_node_t **, devmap_handle_t, int);
static int ext2fs_destroy_node(fs_node_t *);
static int ext2fs_link(fs_node_t *, fs_node_t *, const char *);
static int ext2fs_unlink(fs_node_t *, fs_node_t *, const char *);
static int ext2fs_has_children(bool *, fs_node_t *);
static fs_index_t ext2fs_index_get(fs_node_t *);
static aoff64_t ext2fs_size_get(fs_node_t *);
static unsigned ext2fs_lnkcnt_get(fs_node_t *);
static char ext2fs_plb_get_char(unsigned);
static bool ext2fs_is_directory(fs_node_t *);
static bool ext2fs_is_file(fs_node_t *node);
static devmap_handle_t ext2fs_device_get(fs_node_t *node);

/*
 * Static variables
 */

/**
 * Global hash table holding a mapping from devmap handles to
 * ext2_filesystem_t structures
 */
//TODO
//static hash_table_t fs_hash;

/**
 * Mutex protecting fs_hash
 */
static FIBRIL_MUTEX_INITIALIZE(fs_hash_lock);

/**
 * 
 */
int ext2fs_global_init(void)
{
	//TODO
	//if (!hash_table_create(&fs_hash, FS_BUCKETS, 1, &fs_hash_ops)) {
	//	return ENOMEM;
	//}
	
	return EOK;
}



/*
 * EXT2 libfs operations.
 */

int ext2fs_root_get(fs_node_t **rfn, devmap_handle_t devmap_handle)
{
	// TODO
	return 0;
	//return ext2fs_node_get(rfn, devmap_handle, 0);
}

int ext2fs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	// TODO
	return ENOTSUP;
}

/** Instantiate a EXT2 in-core node. */
int ext2fs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle, fs_index_t index)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_node_open(fs_node_t *fn)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

int ext2fs_node_put(fs_node_t *fn)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_create_node(fs_node_t **rfn, devmap_handle_t devmap_handle, int flags)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_destroy_node(fs_node_t *fn)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	// TODO
	return ENOTSUP;
}

int ext2fs_has_children(bool *has_children, fs_node_t *fn)
{
	// TODO
	return ENOTSUP;
}


fs_index_t ext2fs_index_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

aoff64_t ext2fs_size_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

unsigned ext2fs_lnkcnt_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

char ext2fs_plb_get_char(unsigned pos)
{
	return ext2fs_reg.plb_ro[pos % PLB_SIZE];
}

bool ext2fs_is_directory(fs_node_t *fn)
{
	// TODO
	return false;
}

bool ext2fs_is_file(fs_node_t *fn)
{
	// TODO
	return false;
}

devmap_handle_t ext2fs_device_get(fs_node_t *node)
{
	return 0;
}

/** libfs operations */
libfs_ops_t ext2fs_libfs_ops = {
	.root_get = ext2fs_root_get,
	.match = ext2fs_match,
	.node_get = ext2fs_node_get,
	.node_open = ext2fs_node_open,
	.node_put = ext2fs_node_put,
	.create = ext2fs_create_node,
	.destroy = ext2fs_destroy_node,
	.link = ext2fs_link,
	.unlink = ext2fs_unlink,
	.has_children = ext2fs_has_children,
	.index_get = ext2fs_index_get,
	.size_get = ext2fs_size_get,
	.lnkcnt_get = ext2fs_lnkcnt_get,
	.plb_get_char = ext2fs_plb_get_char,
	.is_directory = ext2fs_is_directory,
	.is_file = ext2fs_is_file,
	.device_get = ext2fs_device_get
};

/*
 * VFS operations.
 */

void ext2fs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	int rc;
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	ext2_filesystem_t *fs;
	
	/* Accept the mount options */
	char *opts;
	rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);
	
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	free(opts);
	
	/* Allocate libext2 filesystem structure */
	fs = (ext2_filesystem_t *) malloc(sizeof(ext2_filesystem_t));
	if (fs == NULL) {
		async_answer_0(rid, ENOMEM);
		return;
	}
	
	/* Initialize the filesystem  */
	rc = ext2_filesystem_init(fs, devmap_handle);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	
	/* Do some sanity checking */
	rc = ext2_filesystem_check_sanity(fs);
	if (rc != EOK) {
		ext2_filesystem_fini(fs);
		async_answer_0(rid, rc);
		return;
	}
	
	
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&ext2fs_libfs_ops, ext2fs_reg.fs_handle, rid, request);
}

void ext2fs_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&ext2fs_libfs_ops, rid, request);
}

void ext2fs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&ext2fs_libfs_ops, ext2fs_reg.fs_handle, rid, request);
}

void ext2fs_read(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
//	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
//	aoff64_t pos =
//	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request), IPC_GET_ARG4(*request));
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_write(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
//	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
//	aoff64_t pos =
//	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request), IPC_GET_ARG4(*request));
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
//	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
//	aoff64_t size =
//	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request), IPC_GET_ARG4(*request));
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_close(ipc_callid_t rid, ipc_call_t *request)
{
	async_answer_0(rid, EOK);
}

void ext2fs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t)IPC_GET_ARG1(*request);
//	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

void ext2fs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&ext2fs_libfs_ops, ext2fs_reg.fs_handle, rid, request);
}

void ext2fs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&ext2fs_libfs_ops, ext2fs_reg.fs_handle, rid, request);
}

void ext2fs_sync(ipc_callid_t rid, ipc_call_t *request)
{
//	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
//	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
	// TODO
	async_answer_0(rid, ENOTSUP);
}

/**
 * @}
 */
