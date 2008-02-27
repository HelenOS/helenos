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

/*
 * For now, we don't distinguish between different dev_handles/instances. All
 * requests resolve to the only instance, rooted in the following variable.
 */
static tmpfs_dentry_t *root;

/*
 * Implementation of the libfs interface.
 */

/* Forward declarations of static functions. */
static bool tmpfs_match(void *, const char *);
static void *tmpfs_create_node(int);
static bool tmpfs_link_node(void *, void *, const char *);
static int tmpfs_unlink_node(void *);
static void tmpfs_destroy_node(void *);

/* Implementation of helper functions. */
static unsigned long tmpfs_index_get(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->index;
}

static unsigned long tmpfs_size_get(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->size;
}

static unsigned tmpfs_lnkcnt_get(void *nodep)
{
	return 1;
}

static void *tmpfs_child_get(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->child;
}

static void *tmpfs_sibling_get(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->sibling;
}

static void *tmpfs_root_get(void)
{
	return root; 
}

static char tmpfs_plb_get_char(unsigned pos)
{
	return tmpfs_reg.plb_ro[pos % PLB_SIZE];
}

static bool tmpfs_is_directory(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->type == TMPFS_DIRECTORY;
}

static bool tmpfs_is_file(void *nodep)
{
	return ((tmpfs_dentry_t *) nodep)->type == TMPFS_FILE;
}

/** libfs operations */
libfs_ops_t tmpfs_libfs_ops = {
	.match = tmpfs_match,
	.create = tmpfs_create_node,
	.destroy = tmpfs_destroy_node,
	.link = tmpfs_link_node,
	.unlink = tmpfs_unlink_node,
	.index_get = tmpfs_index_get,
	.size_get = tmpfs_size_get,
	.lnkcnt_get = tmpfs_lnkcnt_get,
	.child_get = tmpfs_child_get,
	.sibling_get = tmpfs_sibling_get,
	.root_get = tmpfs_root_get,
	.plb_get_char = tmpfs_plb_get_char,
	.is_directory = tmpfs_is_directory,
	.is_file = tmpfs_is_file
};

/** Hash table of all directory entries. */
hash_table_t dentries;

/* Implementation of hash table interface. */
static hash_index_t dentries_hash(unsigned long *key)
{
	return *key % DENTRIES_BUCKETS;
}

static int dentries_compare(unsigned long *key, hash_count_t keys,
    link_t *item)
{
	tmpfs_dentry_t *dentry = hash_table_get_instance(item, tmpfs_dentry_t,
	    dh_link);
	return dentry->index == *key;
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

unsigned tmpfs_next_index = 1;

static void tmpfs_dentry_initialize(tmpfs_dentry_t *dentry)
{
	dentry->index = 0;
	dentry->parent = NULL;
	dentry->sibling = NULL;
	dentry->child = NULL;
	dentry->name = NULL;
	dentry->type = TMPFS_NONE;
	dentry->size = 0;
	dentry->data = NULL;
	link_initialize(&dentry->dh_link);
}

static bool tmpfs_init(void)
{
	if (!hash_table_create(&dentries, DENTRIES_BUCKETS, 1, &dentries_ops))
		return false;
	root = (tmpfs_dentry_t *) tmpfs_create_node(L_DIRECTORY);
	return root != NULL;
}

/** Compare one component of path to a directory entry.
 *
 * @param nodep		Node to compare the path component with.
 * @param component	Array of characters holding component name.
 *
 * @return		True on match, false otherwise.
 */
bool tmpfs_match(void *nodep, const char *component)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) nodep;

	return !strcmp(dentry->name, component);
}

void *tmpfs_create_node(int lflag)
{
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	tmpfs_dentry_t *node = malloc(sizeof(tmpfs_dentry_t));
	if (!node)
		return NULL;

	tmpfs_dentry_initialize(node);
	node->index = tmpfs_next_index++;
	if (lflag & L_DIRECTORY) 
		node->type = TMPFS_DIRECTORY;
	else 
		node->type = TMPFS_FILE;

	/* Insert the new node into the dentry hash table. */
	hash_table_insert(&dentries, &node->index, &node->dh_link);
	return (void *) node;
}

bool tmpfs_link_node(void *prnt, void *chld, const char *nm)
{
	tmpfs_dentry_t *parentp = (tmpfs_dentry_t *) prnt;
	tmpfs_dentry_t *childp = (tmpfs_dentry_t *) chld;

	assert(parentp->type == TMPFS_DIRECTORY);

	size_t len = strlen(nm);
	char *name = malloc(len + 1);
	if (!name)
		return false;
	strcpy(name, nm);
	childp->name = name;

	/* Insert the new node into the namespace. */
	if (parentp->child) {
		tmpfs_dentry_t *tmp = parentp->child;
		while (tmp->sibling)
			tmp = tmp->sibling;
		tmp->sibling = childp;
	} else {
		parentp->child = childp;
	}
	childp->parent = parentp;

	return true;
}

int tmpfs_unlink_node(void *nodeptr)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *)nodeptr;

	if (dentry->child)
		return ENOTEMPTY;

	if (!dentry->parent)
		return EBUSY;

	if (dentry->parent->child == dentry) {
		dentry->parent->child = dentry->sibling;
	} else {
		/* TODO: consider doubly linked list for organizing siblings. */
		tmpfs_dentry_t *tmp = dentry->parent->child;
		while (tmp->sibling != dentry)
			tmp = tmp->sibling;
		tmp->sibling = dentry->sibling;
	}
	dentry->sibling = NULL;
	dentry->parent = NULL;

	free(dentry->name);
	dentry->name = NULL;

	return EOK;
}

void tmpfs_destroy_node(void *nodep)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) nodep;
	
	assert(!dentry->child);
	assert(!dentry->sibling);

	unsigned long index = dentry->index;
	hash_table_remove(&dentries, &index, 1);

	if (dentry->type == TMPFS_FILE)
		free(dentry->data);
	free(dentry);
}

void tmpfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	/* Initialize TMPFS. */
	if (!root && !tmpfs_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	libfs_lookup(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	int dev_handle = IPC_GET_ARG1(*request);
	unsigned long index = IPC_GET_ARG2(*request);
	off_t pos = IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	hlp = hash_table_find(&dentries, &index);
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
	size_t len;
	if (!ipc_data_read_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	size_t bytes;
	if (dentry->type == TMPFS_FILE) {
		bytes = max(0, min(dentry->size - pos, len));
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

		(void) ipc_data_read_finalize(callid, cur->name,
		    strlen(cur->name) + 1);
		bytes = 1;
	}

	/*
	 * Answer the VFS_READ call.
	 */
	ipc_answer_1(rid, EOK, bytes);
}

void tmpfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	int dev_handle = IPC_GET_ARG1(*request);
	unsigned long index = IPC_GET_ARG2(*request);
	off_t pos = IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	hlp = hash_table_find(&dentries, &index);
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
	size_t len;
	if (!ipc_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Check whether the file needs to grow.
	 */
	if (pos + len <= dentry->size) {
		/* The file size is not changing. */
		(void) ipc_data_write_finalize(callid, dentry->data + pos, len);
		ipc_answer_2(rid, EOK, len, dentry->size);
		return;
	}
	size_t delta = (pos + len) - dentry->size;
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
	(void) ipc_data_write_finalize(callid, dentry->data + pos, len);
	ipc_answer_2(rid, EOK, len, dentry->size);
}

void tmpfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	int dev_handle = IPC_GET_ARG1(*request);
	unsigned long index = IPC_GET_ARG2(*request);
	size_t size = IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective dentry.
	 */
	link_t *hlp;
	hlp = hash_table_find(&dentries, &index);
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
	int dev_handle = IPC_GET_ARG1(*request);
	unsigned long index = IPC_GET_ARG2(*request);

	link_t *hlp;
	hlp = hash_table_find(&dentries, &index);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_dentry_t *dentry = hash_table_get_instance(hlp, tmpfs_dentry_t,
	    dh_link);
	tmpfs_destroy_node(dentry);
	ipc_answer_0(rid, EOK);
}

/**
 * @}
 */ 
