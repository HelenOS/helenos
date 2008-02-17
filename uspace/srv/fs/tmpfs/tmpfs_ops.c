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
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <libadt/hash_table.h>
#include <as.h>

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#define PLB_GET_CHAR(i)		(tmpfs_reg.plb_ro[(i) % PLB_SIZE])

#define DENTRIES_BUCKETS	256

#define TMPFS_GET_INDEX(x)	(((tmpfs_dentry_t *)(x))->index)
#define TMPFS_GET_LNKCNT(x)	1

/*
 * Hash table of all directory entries.
 */
hash_table_t dentries;

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

/*
 * For now, we don't distinguish between different dev_handles/instances. All
 * requests resolve to the only instance, rooted in the following variable.
 */
static tmpfs_dentry_t *root;

static bool tmpfs_init(void)
{
	if (!hash_table_create(&dentries, DENTRIES_BUCKETS, 1, &dentries_ops))
		return false;

	root = (tmpfs_dentry_t *) malloc(sizeof(tmpfs_dentry_t));
	if (!root)
		return false;
	tmpfs_dentry_initialize(root);
	root->index = tmpfs_next_index++;
	root->name = "";
	root->type = TMPFS_DIRECTORY;
	hash_table_insert(&dentries, &root->index, &root->dh_link);

	return true;
}

/** Compare one component of path to a directory entry.
 *
 * @param nodep		Node to compare the path component with.
 * @param component	Array of characters holding component name.
 *
 * @return		True on match, false otherwise.
 */
static bool match_component(void *nodep, const char *component)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) nodep;

	return !strcmp(dentry->name, component);
}

static void *create_node(void *nodep,
    const char *component, int lflag)
{
	tmpfs_dentry_t *dentry = (tmpfs_dentry_t *) nodep;

	assert(dentry->type == TMPFS_DIRECTORY);
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	tmpfs_dentry_t *node = malloc(sizeof(tmpfs_dentry_t));
	if (!node)
		return NULL;
	size_t len = strlen(component);
	char *name = malloc(len + 1);
	if (!name) {
		free(node);
		return NULL;
	}
	strcpy(name, component);

	tmpfs_dentry_initialize(node);
	node->index = tmpfs_next_index++;
	node->name = name;
	node->parent = dentry;
	if (lflag & L_DIRECTORY) 
		node->type = TMPFS_DIRECTORY;
	else 
		node->type = TMPFS_FILE;

	/* Insert the new node into the namespace. */
	if (dentry->child) {
		tmpfs_dentry_t *tmp = dentry->child;
		while (tmp->sibling)
			tmp = tmp->sibling;
		tmp->sibling = node;
	} else {
		dentry->child = node;
	}

	/* Insert the new node into the dentry hash table. */
	hash_table_insert(&dentries, &node->index, &node->dh_link);
	return (void *) node;
}

static int destroy_component(void *nodeptr)
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

	return EOK;
}

void tmpfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	unsigned next = IPC_GET_ARG1(*request);
	unsigned last = IPC_GET_ARG2(*request);
	int dev_handle = IPC_GET_ARG3(*request);
	int lflag = IPC_GET_ARG4(*request);

	if (last < next)
		last += PLB_SIZE;

	/*
	 * Initialize TMPFS.
	 */
	if (!root && !tmpfs_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	tmpfs_dentry_t *dtmp = root->child; 
	tmpfs_dentry_t *dcur = root;

	if (PLB_GET_CHAR(next) == '/')
		next++;		/* eat slash */
	
	char component[NAME_MAX + 1];
	int len = 0;
	while (dtmp && next <= last) {

		/* collect the component */
		if (PLB_GET_CHAR(next) != '/') {
			if (len + 1 == NAME_MAX) {
				/* comopnent length overflow */
				ipc_answer_0(rid, ENAMETOOLONG);
				return;
			}
			component[len++] = PLB_GET_CHAR(next);
			next++;	/* process next character */
			if (next <= last) 
				continue;
		}

		assert(len);
		component[len] = '\0';
		next++;		/* eat slash */
		len = 0;

		/* match the component */
		while (dtmp && !match_component(dtmp, component))
			dtmp = dtmp->sibling;

		/* handle miss: match amongst siblings */
		if (!dtmp) {
			if ((next > last) && (lflag & L_CREATE)) {
				/* no components left and L_CREATE specified */
				if (dcur->type != TMPFS_DIRECTORY) {
					ipc_answer_0(rid, ENOTDIR);
					return;
				} 
				void *nodep = create_node(dcur,
				    component, lflag);
				if (nodep) {
					ipc_answer_5(rid, EOK,
					    tmpfs_reg.fs_handle, dev_handle,
					    TMPFS_GET_INDEX(nodep), 0,
					    TMPFS_GET_LNKCNT(nodep));
				} else {
					ipc_answer_0(rid, ENOSPC);
				}
				return;
			}
			ipc_answer_0(rid, ENOENT);
			return;
		}

		/* descend one level */
		dcur = dtmp;
		dtmp = dtmp->child;
	}

	/* handle miss: excessive components */
	if (!dtmp && next <= last) {
		if (lflag & L_CREATE) {
			if (dcur->type != TMPFS_DIRECTORY) {
				ipc_answer_0(rid, ENOTDIR);
				return;
			}

			/* collect next component */
			while (next <= last) {
				if (PLB_GET_CHAR(next) == '/') {
					/* more than one component */
					ipc_answer_0(rid, ENOENT);
					return;
				}
				if (len + 1 == NAME_MAX) {
					/* component length overflow */
					ipc_answer_0(rid, ENAMETOOLONG);
					return;
				}
				component[len++] = PLB_GET_CHAR(next);
				next++;	/* process next character */
			}
			assert(len);
			component[len] = '\0';
			len = 0;
				
			void *nodep = create_node(dcur, component, lflag);
			if (nodep) {
				ipc_answer_5(rid, EOK, tmpfs_reg.fs_handle,
				    dev_handle, TMPFS_GET_INDEX(nodep), 0,
				    TMPFS_GET_LNKCNT(nodep));
			} else {
				ipc_answer_0(rid, ENOSPC);
			}
			return;
		}
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/* handle hit */
	if (lflag & L_DESTROY) {
		unsigned old_lnkcnt = TMPFS_GET_LNKCNT(dcur);
		int res = destroy_component(dcur);
		ipc_answer_5(rid, (ipcarg_t)res, tmpfs_reg.fs_handle,
		    dev_handle, dcur->index, dcur->size, old_lnkcnt);
		return;
	}
	if ((lflag & (L_CREATE | L_EXCLUSIVE)) == (L_CREATE | L_EXCLUSIVE)) {
		ipc_answer_0(rid, EEXIST);
		return;
	}
	if ((lflag & L_FILE) && (dcur->type != TMPFS_FILE)) {
		ipc_answer_0(rid, EISDIR);
		return;
	}
	if ((lflag & L_DIRECTORY) && (dcur->type != TMPFS_DIRECTORY)) {
		ipc_answer_0(rid, ENOTDIR);
		return;
	}

	ipc_answer_5(rid, EOK, tmpfs_reg.fs_handle, dev_handle, dcur->index,
	    dcur->size, TMPFS_GET_LNKCNT(dcur));
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
		tmpfs_dentry_t *cur = dentry->child;
		
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

void tmpfs_free(ipc_callid_t rid, ipc_call_t *request)
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
	
	assert(!dentry->parent);
	assert(!dentry->child);
	assert(!dentry->sibling);

	hash_table_remove(&dentries, &index, 1);

	if (dentry->type == TMPFS_FILE)
		free(dentry->data);
	free(dentry->name);
	free(dentry);

	ipc_answer_0(rid, EOK);
}

/**
 * @}
 */ 
