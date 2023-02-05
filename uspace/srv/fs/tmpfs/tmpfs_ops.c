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

/** @addtogroup tmpfs
 * @{
 */

/**
 * @file	tmpfs_ops.c
 * @brief	Implementation of VFS operations for the TMPFS file system
 *		server.
 */

#include "tmpfs.h"
#include "../../vfs/vfs.h"
#include <macros.h>
#include <stdint.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <as.h>
#include <libfs.h>

/** All root nodes have index 0. */
#define TMPFS_SOME_ROOT  0

/** Global counter for assigning node indices. Shared by all instances. */
fs_index_t tmpfs_next_index = 1;

/*
 * Implementation of the libfs interface.
 */

/* Forward declarations of static functions. */
static errno_t tmpfs_match(fs_node_t **, fs_node_t *, const char *);
static errno_t tmpfs_node_get(fs_node_t **, service_id_t, fs_index_t);
static errno_t tmpfs_node_open(fs_node_t *);
static errno_t tmpfs_node_put(fs_node_t *);
static errno_t tmpfs_create_node(fs_node_t **, service_id_t, int);
static errno_t tmpfs_destroy_node(fs_node_t *);
static errno_t tmpfs_link_node(fs_node_t *, fs_node_t *, const char *);
static errno_t tmpfs_unlink_node(fs_node_t *, fs_node_t *, const char *);

/* Implementation of helper functions. */
static errno_t tmpfs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return tmpfs_node_get(rfn, service_id, TMPFS_SOME_ROOT);
}

static errno_t tmpfs_has_children(bool *has_children, fs_node_t *fn)
{
	*has_children = !list_empty(&TMPFS_NODE(fn)->cs_list);
	return EOK;
}

static fs_index_t tmpfs_index_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->index;
}

static aoff64_t tmpfs_size_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->size;
}

static unsigned tmpfs_lnkcnt_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->lnkcnt;
}

static bool tmpfs_is_directory(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_DIRECTORY;
}

static bool tmpfs_is_file(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_FILE;
}

static service_id_t tmpfs_service_get(fs_node_t *fn)
{
	return 0;
}

/** libfs operations */
libfs_ops_t tmpfs_libfs_ops = {
	.root_get = tmpfs_root_get,
	.match = tmpfs_match,
	.node_get = tmpfs_node_get,
	.node_open = tmpfs_node_open,
	.node_put = tmpfs_node_put,
	.create = tmpfs_create_node,
	.destroy = tmpfs_destroy_node,
	.link = tmpfs_link_node,
	.unlink = tmpfs_unlink_node,
	.has_children = tmpfs_has_children,
	.index_get = tmpfs_index_get,
	.size_get = tmpfs_size_get,
	.lnkcnt_get = tmpfs_lnkcnt_get,
	.is_directory = tmpfs_is_directory,
	.is_file = tmpfs_is_file,
	.service_get = tmpfs_service_get
};

/** Hash table of all TMPFS nodes. */
hash_table_t nodes;

/*
 * Implementation of hash table interface for the nodes hash table.
 */

typedef struct {
	service_id_t service_id;
	fs_index_t index;
} node_key_t;

static size_t nodes_key_hash(const void *k)
{
	const node_key_t *key = k;
	return hash_combine(key->service_id, key->index);
}

static size_t nodes_hash(const ht_link_t *item)
{
	tmpfs_node_t *nodep = hash_table_get_inst(item, tmpfs_node_t, nh_link);
	return hash_combine(nodep->service_id, nodep->index);
}

static bool nodes_key_equal(const void *key_arg, const ht_link_t *item)
{
	tmpfs_node_t *node = hash_table_get_inst(item, tmpfs_node_t, nh_link);
	const node_key_t *key = key_arg;

	return key->service_id == node->service_id && key->index == node->index;
}

static void nodes_remove_callback(ht_link_t *item)
{
	tmpfs_node_t *nodep = hash_table_get_inst(item, tmpfs_node_t, nh_link);

	while (!list_empty(&nodep->cs_list)) {
		tmpfs_dentry_t *dentryp = list_get_instance(
		    list_first(&nodep->cs_list), tmpfs_dentry_t, link);

		assert(nodep->type == TMPFS_DIRECTORY);
		list_remove(&dentryp->link);
		free(dentryp);
	}

	if (nodep->data) {
		assert(nodep->type == TMPFS_FILE);
		free(nodep->data);
	}
	free(nodep->bp);
	free(nodep);
}

/** TMPFS nodes hash table operations. */
const hash_table_ops_t nodes_ops = {
	.hash = nodes_hash,
	.key_hash = nodes_key_hash,
	.key_equal = nodes_key_equal,
	.equal = NULL,
	.remove_callback = nodes_remove_callback
};

static void tmpfs_node_initialize(tmpfs_node_t *nodep)
{
	nodep->bp = NULL;
	nodep->index = 0;
	nodep->service_id = 0;
	nodep->type = TMPFS_NONE;
	nodep->lnkcnt = 0;
	nodep->size = 0;
	nodep->data = NULL;
	list_initialize(&nodep->cs_list);
}

static void tmpfs_dentry_initialize(tmpfs_dentry_t *dentryp)
{
	link_initialize(&dentryp->link);
	dentryp->name = NULL;
	dentryp->node = NULL;
}

bool tmpfs_init(void)
{
	if (!hash_table_create(&nodes, 0, 0, &nodes_ops))
		return false;

	return true;
}

static bool tmpfs_instance_init(service_id_t service_id)
{
	fs_node_t *rfn;
	errno_t rc;

	rc = tmpfs_create_node(&rfn, service_id, L_DIRECTORY);
	if (rc != EOK || !rfn)
		return false;
	TMPFS_NODE(rfn)->lnkcnt = 0;	/* FS root is not linked */
	return true;
}

static bool rm_service_id_nodes(ht_link_t *item, void *arg)
{
	service_id_t sid = *(service_id_t *)arg;
	tmpfs_node_t *node = hash_table_get_inst(item, tmpfs_node_t, nh_link);

	if (node->service_id == sid) {
		hash_table_remove_item(&nodes, &node->nh_link);
	}
	return true;
}

static void tmpfs_instance_done(service_id_t service_id)
{
	hash_table_apply(&nodes, rm_service_id_nodes, &service_id);
}

errno_t tmpfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);

	list_foreach(parentp->cs_list, link, tmpfs_dentry_t, dentryp) {
		if (!str_cmp(dentryp->name, component)) {
			*rfn = FS_NODE(dentryp->node);
			return EOK;
		}
	}

	*rfn = NULL;
	return EOK;
}

errno_t tmpfs_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	node_key_t key = {
		.service_id = service_id,
		.index = index
	};

	ht_link_t *lnk = hash_table_find(&nodes, &key);

	if (lnk) {
		tmpfs_node_t *nodep;
		nodep = hash_table_get_inst(lnk, tmpfs_node_t, nh_link);
		*rfn = FS_NODE(nodep);
	} else {
		*rfn = NULL;
	}
	return EOK;
}

errno_t tmpfs_node_open(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

errno_t tmpfs_node_put(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

errno_t tmpfs_create_node(fs_node_t **rfn, service_id_t service_id, int lflag)
{
	fs_node_t *rootfn;
	errno_t rc;

	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	tmpfs_node_t *nodep = malloc(sizeof(tmpfs_node_t));
	if (!nodep)
		return ENOMEM;

	tmpfs_node_initialize(nodep);
	nodep->bp = malloc(sizeof(fs_node_t));
	if (!nodep->bp) {
		free(nodep);
		return ENOMEM;
	}

	fs_node_initialize(nodep->bp);
	nodep->bp->data = nodep;  /* Link the FS and TMPFS nodes */

	rc = tmpfs_root_get(&rootfn, service_id);
	assert(rc == EOK);
	if (!rootfn)
		nodep->index = TMPFS_SOME_ROOT;
	else
		nodep->index = tmpfs_next_index++;

	nodep->service_id = service_id;
	if (lflag & L_DIRECTORY)
		nodep->type = TMPFS_DIRECTORY;
	else
		nodep->type = TMPFS_FILE;

	/* Insert the new node into the nodes hash table. */
	hash_table_insert(&nodes, &nodep->nh_link);
	*rfn = FS_NODE(nodep);
	return EOK;
}

errno_t tmpfs_destroy_node(fs_node_t *fn)
{
	tmpfs_node_t *nodep = TMPFS_NODE(fn);

	assert(!nodep->lnkcnt);
	assert(list_empty(&nodep->cs_list));

	hash_table_remove_item(&nodes, &nodep->nh_link);

	/*
	 * The nodes_remove_callback() function takes care of the actual
	 * resource deallocation.
	 */
	return EOK;
}

errno_t tmpfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);
	tmpfs_node_t *childp = TMPFS_NODE(cfn);
	tmpfs_dentry_t *dentryp;

	assert(parentp->type == TMPFS_DIRECTORY);

	/* Check for duplicit entries. */
	list_foreach(parentp->cs_list, link, tmpfs_dentry_t, dp) {
		if (!str_cmp(dp->name, nm))
			return EEXIST;
	}

	/* Allocate and initialize the dentry. */
	dentryp = malloc(sizeof(tmpfs_dentry_t));
	if (!dentryp)
		return ENOMEM;
	tmpfs_dentry_initialize(dentryp);

	/* Populate and link the new dentry. */
	size_t size = str_size(nm);
	dentryp->name = malloc(size + 1);
	if (!dentryp->name) {
		free(dentryp);
		return ENOMEM;
	}
	str_cpy(dentryp->name, size + 1, nm);
	dentryp->node = childp;
	childp->lnkcnt++;
	list_append(&dentryp->link, &parentp->cs_list);

	return EOK;
}

errno_t tmpfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);
	tmpfs_node_t *childp = NULL;
	tmpfs_dentry_t *dentryp;

	if (!parentp)
		return EBUSY;

	list_foreach(parentp->cs_list, link, tmpfs_dentry_t, dp) {
		if (!str_cmp(dp->name, nm)) {
			dentryp = dp;
			childp = dentryp->node;
			assert(FS_NODE(childp) == cfn);
			break;
		}
	}

	if (!childp)
		return ENOENT;

	if ((childp->lnkcnt == 1) && !list_empty(&childp->cs_list))
		return ENOTEMPTY;

	list_remove(&dentryp->link);
	free(dentryp);
	childp->lnkcnt--;

	return EOK;
}

/*
 * Implementation of the VFS_OUT interface.
 */

static errno_t tmpfs_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	return ENOTSUP;
}

static errno_t
tmpfs_mounted(service_id_t service_id, const char *opts, fs_index_t *index,
    aoff64_t *size)
{
	fs_node_t *rootfn;
	errno_t rc;

	/* Check if this device is not already mounted. */
	rc = tmpfs_root_get(&rootfn, service_id);
	if ((rc == EOK) && (rootfn)) {
		(void) tmpfs_node_put(rootfn);
		return EEXIST;
	}

	/* Initialize TMPFS instance. */
	if (!tmpfs_instance_init(service_id))
		return ENOMEM;

	rc = tmpfs_root_get(&rootfn, service_id);
	assert(rc == EOK);
	tmpfs_node_t *rootp = TMPFS_NODE(rootfn);

	*index = rootp->index;
	*size = rootp->size;

	return EOK;
}

static errno_t tmpfs_unmounted(service_id_t service_id)
{
	tmpfs_instance_done(service_id);
	return EOK;
}

static errno_t tmpfs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	/*
	 * Lookup the respective TMPFS node.
	 */
	node_key_t key = {
		.service_id = service_id,
		.index = index
	};

	ht_link_t *hlp = hash_table_find(&nodes, &key);
	if (!hlp)
		return ENOENT;

	tmpfs_node_t *nodep = hash_table_get_inst(hlp, tmpfs_node_t, nh_link);

	/*
	 * Receive the read request.
	 */
	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EINVAL);
		return EINVAL;
	}

	size_t bytes;
	if (nodep->type == TMPFS_FILE) {
		bytes = min(nodep->size - pos, size);
		(void) async_data_read_finalize(&call, nodep->data + pos,
		    bytes);
	} else {
		tmpfs_dentry_t *dentryp;
		link_t *lnk;

		assert(nodep->type == TMPFS_DIRECTORY);

		/*
		 * Yes, we really use O(n) algorithm here.
		 * If it bothers someone, it could be fixed by introducing a
		 * hash table.
		 */
		lnk = list_nth(&nodep->cs_list, pos);

		if (lnk == NULL) {
			async_answer_0(&call, ENOENT);
			return ENOENT;
		}

		dentryp = list_get_instance(lnk, tmpfs_dentry_t, link);

		(void) async_data_read_finalize(&call, dentryp->name,
		    str_size(dentryp->name) + 1);
		bytes = 1;
	}

	*rbytes = bytes;
	return EOK;
}

static errno_t
tmpfs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	/*
	 * Lookup the respective TMPFS node.
	 */
	node_key_t key = {
		.service_id = service_id,
		.index = index
	};

	ht_link_t *hlp = hash_table_find(&nodes, &key);

	if (!hlp)
		return ENOENT;

	tmpfs_node_t *nodep = hash_table_get_inst(hlp, tmpfs_node_t, nh_link);

	/*
	 * Receive the write request.
	 */
	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EINVAL);
		return EINVAL;
	}

	/*
	 * Check whether the file needs to grow.
	 */
	if (pos + size <= nodep->size) {
		/* The file size is not changing. */
		(void) async_data_write_finalize(&call, nodep->data + pos,
		    size);
		goto out;
	}
	size_t delta = (pos + size) - nodep->size;
	/*
	 * At this point, we are deliberately extremely straightforward and
	 * simply realloc the contents of the file on every write that grows the
	 * file. In the end, the situation might not be as bad as it may look:
	 * our heap allocator can save us and just grow the block whenever
	 * possible.
	 */
	void *newdata = realloc(nodep->data, nodep->size + delta);
	if (!newdata) {
		async_answer_0(&call, ENOMEM);
		size = 0;
		goto out;
	}
	/* Clear any newly allocated memory in order to emulate gaps. */
	memset(newdata + nodep->size, 0, delta);
	nodep->size += delta;
	nodep->data = newdata;
	(void) async_data_write_finalize(&call, nodep->data + pos, size);

out:
	*wbytes = size;
	*nsize = nodep->size;
	return EOK;
}

static errno_t tmpfs_truncate(service_id_t service_id, fs_index_t index,
    aoff64_t size)
{
	/*
	 * Lookup the respective TMPFS node.
	 */
	node_key_t key = {
		.service_id = service_id,
		.index = index
	};

	ht_link_t *hlp = hash_table_find(&nodes, &key);
	if (!hlp)
		return ENOENT;
	tmpfs_node_t *nodep = hash_table_get_inst(hlp, tmpfs_node_t, nh_link);

	if (size == nodep->size)
		return EOK;

	if (size > SIZE_MAX)
		return ENOMEM;

	void *newdata = realloc(nodep->data, size);
	if (!newdata && size)
		return ENOMEM;

	if (size > nodep->size) {
		size_t delta = size - nodep->size;
		memset(newdata + nodep->size, 0, delta);
	}

	nodep->size = size;
	nodep->data = newdata;
	return EOK;
}

static errno_t tmpfs_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}

static errno_t tmpfs_destroy(service_id_t service_id, fs_index_t index)
{
	node_key_t key = {
		.service_id = service_id,
		.index = index
	};

	ht_link_t *hlp = hash_table_find(&nodes, &key);
	if (!hlp)
		return ENOENT;
	tmpfs_node_t *nodep = hash_table_get_inst(hlp, tmpfs_node_t,
	    nh_link);
	return tmpfs_destroy_node(FS_NODE(nodep));
}

static errno_t tmpfs_sync(service_id_t service_id, fs_index_t index)
{
	/*
	 * TMPFS keeps its data structures always consistent,
	 * thus the sync operation is a no-op.
	 */
	return EOK;
}

vfs_out_ops_t tmpfs_ops = {
	.fsprobe = tmpfs_fsprobe,
	.mounted = tmpfs_mounted,
	.unmounted = tmpfs_unmounted,
	.read = tmpfs_read,
	.write = tmpfs_write,
	.truncate = tmpfs_truncate,
	.close = tmpfs_close,
	.destroy = tmpfs_destroy,
	.sync = tmpfs_sync,
};

/**
 * @}
 */
