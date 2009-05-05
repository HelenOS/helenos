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
 * @file	tmpfs_ops.c
 * @brief	Implementation of VFS operations for the TMPFS file system
 *		server.
 */

#include "tmpfs.h"
#include "../../vfs/vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <atomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <libadt/hash_table.h>
#include <as.h>
#include <libfs.h>

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#define DENTRIES_BUCKETS	256

#define NAMES_BUCKETS		4

/** All root nodes have index 0. */
#define TMPFS_SOME_ROOT		0
/** Global counter for assigning node indices. Shared by all instances. */
fs_index_t tmpfs_next_index = 1;

/*
 * Implementation of the libfs interface.
 */

/* Forward declarations of static functions. */
static fs_node_t *tmpfs_match(fs_node_t *, const char *);
static fs_node_t *tmpfs_node_get(dev_handle_t, fs_index_t);
static void tmpfs_node_put(fs_node_t *);
static fs_node_t *tmpfs_create_node(dev_handle_t, int);
static int tmpfs_link_node(fs_node_t *, fs_node_t *, const char *);
static int tmpfs_unlink_node(fs_node_t *, fs_node_t *);
static int tmpfs_destroy_node(fs_node_t *);

/* Implementation of helper functions. */
static fs_index_t tmpfs_index_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->index;
}

static size_t tmpfs_size_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->size;
}

static unsigned tmpfs_lnkcnt_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->lnkcnt;
}

static bool tmpfs_has_children(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->child != NULL;
}

static fs_node_t *tmpfs_root_get(dev_handle_t dev_handle)
{
	return tmpfs_node_get(dev_handle, TMPFS_SOME_ROOT); 
}

static char tmpfs_plb_get_char(unsigned pos)
{
	return tmpfs_reg.plb_ro[pos % PLB_SIZE];
}

static bool tmpfs_is_directory(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_DIRECTORY;
}

static bool tmpfs_is_file(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_FILE;
}

/** libfs operations */
libfs_ops_t tmpfs_libfs_ops = {
	.match = tmpfs_match,
	.node_get = tmpfs_node_get,
	.node_put = tmpfs_node_put,
	.create = tmpfs_create_node,
	.destroy = tmpfs_destroy_node,
	.link = tmpfs_link_node,
	.unlink = tmpfs_unlink_node,
	.index_get = tmpfs_index_get,
	.size_get = tmpfs_size_get,
	.lnkcnt_get = tmpfs_lnkcnt_get,
	.has_children = tmpfs_has_children,
	.root_get = tmpfs_root_get,
	.plb_get_char = tmpfs_plb_get_char,
	.is_directory = tmpfs_is_directory,
	.is_file = tmpfs_is_file
};

/** Hash table of all directory entries. */
hash_table_t dentries;

#define DENTRIES_KEY_INDEX	0
#define DENTRIES_KEY_DEV	1

/* Implementation of hash table interface for the dentries hash table. */
static hash_index_t dentries_hash(unsigned long key[])
{
	return key[DENTRIES_KEY_INDEX] % DENTRIES_BUCKETS;
}

static int dentries_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	tmpfs_dentry_t *dentry = hash_table_get_instance(item, tmpfs_dentry_t,
	    dh_link);
	return (dentry->index == key[DENTRIES_KEY_INDEX] &&
	    dentry->dev_handle == key[DENTRIES_KEY_DEV]);
}

static void dentries_remove_callback(link_t *item)
{
}

/** TMPFS dentries hash table operations. */
hash_table_operations_t dentries_ops = {
	.hash = dentries_hash,
	.compare = dentries_compare,
	.remove_callback = dentries_remove_callback
};

typedef struct {
	char *name;
	tmpfs_dentry_t *parent;
	link_t link;
} tmpfs_name_t;

/* Implementation of hash table interface for the names hash table. */
static hash_index_t names_hash(unsigned long *key)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) *key;
	return dentry->index % NAMES_BUCKETS;
}

static int names_compare(unsigned long *key, hash_count_t keys, link_t *item)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) *key;
	tmpfs_name_t *namep = hash_table_get_instance(item, tmpfs_name_t,
	    link);
	return dentry == namep->parent;
}

static void names_remove_callback(link_t *item)
{
	tmpfs_name_t *namep = hash_table_get_instance(item, tmpfs_name_t,
	    link);
	free(namep->name);
	free(namep);
}

/** TMPFS node names hash table operations. */
static hash_table_operations_t names_ops = {
	.hash = names_hash,
	.compare = names_compare,
	.remove_callback = names_remove_callback
};

static void tmpfs_name_initialize(tmpfs_name_t *namep)
{
	namep->name = NULL;
	namep->parent = NULL;
	link_initialize(&namep->link);
}

static bool tmpfs_dentry_initialize(tmpfs_dentry_t *dentry)
{
	dentry->bp = NULL;
	dentry->index = 0;
	dentry->dev_handle = 0;
	dentry->sibling = NULL;
	dentry->child = NULL;
	dentry->type = TMPFS_NONE;
	dentry->lnkcnt = 0;
	dentry->size = 0;
	dentry->data = NULL;
	link_initialize(&dentry->dh_link);
	return (bool)hash_table_create(&dentry->names, NAMES_BUCKETS, 1,
	    &names_ops);
}

bool tmpfs_init(void)
{
	if (!hash_table_create(&dentries, DENTRIES_BUCKETS, 2, &dentries_ops))
		return false;
	
	return true;
}

static bool tmpfs_instance_init(dev_handle_t dev_handle)
{
	fs_node_t *rfn;
	
	rfn = tmpfs_create_node(dev_handle, L_DIRECTORY);
	if (!rfn) 
		return false;
	TMPFS_NODE(rfn)->lnkcnt = 0;	/* FS root is not linked */
	return true;
}

/** Compare one component of path to a directory entry.
 *
 * @param parentp	Pointer to node from which we descended.
 * @param childp	Pointer to node to compare the path component with.
 * @param component	Array of characters holding component name.
 *
 * @return		True on match, false otherwise.
 */
static bool
tmpfs_match_one(tmpfs_dentry_t *parentp, tmpfs_dentry_t *childp,
    const char *component)
{
	unsigned long key = (unsigned long) parentp;
	link_t *hlp = hash_table_find(&childp->names, &key);
	assert(hlp);
	tmpfs_name_t *namep = hash_table_get_instance(hlp, tmpfs_name_t, link);
	return !str_cmp(namep->name, component);
}

fs_node_t *tmpfs_match(fs_node_t *pfn, const char *component)
{
	tmpfs_dentry_t *parentp = TMPFS_NODE(pfn);
	tmpfs_dentry_t *childp = parentp->child;

	while (childp && !tmpfs_match_one(parentp, childp, component))
		childp = childp->sibling;

	return FS_NODE(childp);
}

fs_node_t *tmpfs_node_get(dev_handle_t dev_handle, fs_index_t index)
{
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = index,
		[DENTRIES_KEY_DEV] = dev_handle
	};
	link_t *lnk = hash_table_find(&dentries, key);
	if (!lnk)
		return NULL;
	return FS_NODE(hash_table_get_instance(lnk, tmpfs_dentry_t, dh_link));
}

void tmpfs_node_put(fs_node_t *fn)
{
	/* nothing to do */
}

fs_node_t *tmpfs_create_node(dev_handle_t dev_handle, int lflag)
{
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	fs_node_t *fn = malloc(sizeof(fs_node_t));
	if (!fn)
		return NULL;
	
	tmpfs_dentry_t *node = malloc(sizeof(tmpfs_dentry_t));
	if (!node) {
		free(fn);
		return NULL;
	}
	if (!tmpfs_dentry_initialize(node)) {
		free(fn);
		free(node);
		return NULL;
	}
	fn->data = node;
	node->bp = fn;	/* establish the back pointer */
	if (!tmpfs_root_get(dev_handle))
		node->index = TMPFS_SOME_ROOT;
	else
		node->index = tmpfs_next_index++;
	node->dev_handle = dev_handle;
	if (lflag & L_DIRECTORY) 
		node->type = TMPFS_DIRECTORY;
	else 
		node->type = TMPFS_FILE;

	/* Insert the new node into the dentry hash table. */
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = node->index,
		[DENTRIES_KEY_DEV] = node->dev_handle
	};
	hash_table_insert(&dentries, key, &node->dh_link);
	return fn;
}

int tmpfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	tmpfs_dentry_t *parentp = TMPFS_NODE(pfn);
	tmpfs_dentry_t *childp = TMPFS_NODE(cfn);

	assert(parentp->type == TMPFS_DIRECTORY);

	tmpfs_name_t *namep = malloc(sizeof(tmpfs_name_t));
	if (!namep)
		return ENOMEM;
	tmpfs_name_initialize(namep);
	size_t size = str_size(nm);
	namep->name = malloc(size + 1);
	if (!namep->name) {
		free(namep);
		return ENOMEM;
	}
	str_cpy(namep->name, size + 1, nm);
	namep->parent = parentp;
	
	childp->lnkcnt++;

	unsigned long key = (unsigned long) parentp;
	hash_table_insert(&childp->names, &key, &namep->link);

	/* Insert the new node into the namespace. */
	if (parentp->child) {
		tmpfs_dentry_t *tmp = parentp->child;
		while (tmp->sibling)
			tmp = tmp->sibling;
		tmp->sibling = childp;
	} else {
		parentp->child = childp;
	}

	return EOK;
}

int tmpfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn)
{
	tmpfs_dentry_t *parentp = TMPFS_NODE(pfn);
	tmpfs_dentry_t *childp = TMPFS_NODE(cfn);

	if (!parentp)
		return EBUSY;

	if (childp->child)
		return ENOTEMPTY;

	if (parentp->child == childp) {
		parentp->child = childp->sibling;
	} else {
		/* TODO: consider doubly linked list for organizing siblings. */
		tmpfs_dentry_t *tmp = parentp->child;
		while (tmp->sibling != childp)
			tmp = tmp->sibling;
		tmp->sibling = childp->sibling;
	}
	childp->sibling = NULL;

	unsigned long key = (unsigned long) parentp;
	hash_table_remove(&childp->names, &key, 1);

	childp->lnkcnt--;

	return EOK;
}

int tmpfs_destroy_node(fs_node_t *fn)
{
	tmpfs_dentry_t *dentry = TMPFS_NODE(fn);
	
	assert(!dentry->lnkcnt);
	assert(!dentry->child);
	assert(!dentry->sibling);

	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = dentry->index,
		[DENTRIES_KEY_DEV] = dentry->dev_handle
	};
	hash_table_remove(&dentries, key, 2);

	hash_table_destroy(&dentry->names);

	if (dentry->type == TMPFS_FILE)
		free(dentry->data);
	free(dentry->bp);
	free(dentry);
	return EOK;
}

void tmpfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);

	/* accept the mount options */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *opts = malloc(size + 1);
	if (!opts) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	ipcarg_t retval = ipc_data_write_finalize(callid, opts, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(opts);
		return;
	}
	opts[size] = '\0';

	/* Initialize TMPFS instance. */
	if (!tmpfs_instance_init(dev_handle)) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	tmpfs_dentry_t *root = TMPFS_NODE(tmpfs_root_get(dev_handle));
	if (str_cmp(opts, "restore") == 0) {
		if (tmpfs_restore(dev_handle))
			ipc_answer_3(rid, EOK, root->index, root->size,
			    root->lnkcnt);
		else
			ipc_answer_0(rid, ELIMIT);
	} else {
		ipc_answer_3(rid, EOK, root->index, root->size, root->lnkcnt);
	}
}

void tmpfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t mp_dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fs_index_t mp_index = (fs_index_t) IPC_GET_ARG2(*request);
	fs_handle_t mr_fs_handle = (fs_handle_t) IPC_GET_ARG3(*request);
	dev_handle_t mr_dev_handle = (dev_handle_t) IPC_GET_ARG4(*request);
	
	ipc_answer_0(rid, ENOTSUP);
}

void tmpfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = index,
		[DENTRIES_KEY_DEV] = dev_handle,
	};
	hlp = hash_table_find(&dentries, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_dentry_t *dentry = hash_table_get_instance(hlp, tmpfs_dentry_t,
	    dh_link);

	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_read_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	size_t bytes;
	if (dentry->type == TMPFS_FILE) {
		bytes = max(0, min(dentry->size - pos, size));
		(void) ipc_data_read_finalize(callid, dentry->data + pos,
		    bytes);
	} else {
		int i;
		tmpfs_dentry_t *cur;
		
		assert(dentry->type == TMPFS_DIRECTORY);
		
		/*
		 * Yes, we really use O(n) algorithm here.
		 * If it bothers someone, it could be fixed by introducing a
		 * hash table.
		 */
		for (i = 0, cur = dentry->child; i < pos && cur; i++,
		    cur = cur->sibling)
			;

		if (!cur) {
			ipc_answer_0(callid, ENOENT);
			ipc_answer_1(rid, ENOENT, 0);
			return;
		}

		unsigned long key = (unsigned long) dentry;
		link_t *hlp = hash_table_find(&cur->names, &key);
		assert(hlp);
		tmpfs_name_t *namep = hash_table_get_instance(hlp, tmpfs_name_t,
		    link);

		(void) ipc_data_read_finalize(callid, namep->name,
		    str_size(namep->name) + 1);
		bytes = 1;
	}

	/*
	 * Answer the VFS_READ call.
	 */
	ipc_answer_1(rid, EOK, bytes);
}

void tmpfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = index,
		[DENTRIES_KEY_DEV] = dev_handle
	};
	hlp = hash_table_find(&dentries, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_dentry_t *dentry = hash_table_get_instance(hlp, tmpfs_dentry_t,
	    dh_link);

	/*
	 * Receive the write request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Check whether the file needs to grow.
	 */
	if (pos + size <= dentry->size) {
		/* The file size is not changing. */
		(void) ipc_data_write_finalize(callid, dentry->data + pos, size);
		ipc_answer_2(rid, EOK, size, dentry->size);
		return;
	}
	size_t delta = (pos + size) - dentry->size;
	/*
	 * At this point, we are deliberately extremely straightforward and
	 * simply realloc the contents of the file on every write that grows the
	 * file. In the end, the situation might not be as bad as it may look:
	 * our heap allocator can save us and just grow the block whenever
	 * possible.
	 */
	void *newdata = realloc(dentry->data, dentry->size + delta);
	if (!newdata) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_2(rid, EOK, 0, dentry->size);
		return;
	}
	/* Clear any newly allocated memory in order to emulate gaps. */
	memset(newdata + dentry->size, 0, delta);
	dentry->size += delta;
	dentry->data = newdata;
	(void) ipc_data_write_finalize(callid, dentry->data + pos, size);
	ipc_answer_2(rid, EOK, size, dentry->size);
}

void tmpfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	size_t size = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = index,
		[DENTRIES_KEY_DEV] = dev_handle
	};
	hlp = hash_table_find(&dentries, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_dentry_t *dentry = hash_table_get_instance(hlp, tmpfs_dentry_t,
	    dh_link);

	if (size == dentry->size) {
		ipc_answer_0(rid, EOK);
		return;
	}

	void *newdata = realloc(dentry->data, size);
	if (!newdata) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	if (size > dentry->size) {
		size_t delta = size - dentry->size;
		memset(newdata + dentry->size, 0, delta);
	}
	dentry->size = size;
	dentry->data = newdata;
	ipc_answer_0(rid, EOK);
}

void tmpfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	int rc;

	link_t *hlp;
	unsigned long key[] = {
		[DENTRIES_KEY_INDEX] = index,
		[DENTRIES_KEY_DEV] = dev_handle
	};
	hlp = hash_table_find(&dentries, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_dentry_t *dentry = hash_table_get_instance(hlp, tmpfs_dentry_t,
	    dh_link);
	rc = tmpfs_destroy_node(FS_NODE(dentry));
	ipc_answer_0(rid, rc);
}

/**
 * @}
 */ 
