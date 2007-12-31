/*
 * Copyright (c) 2007 Jakub Jermar
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
#include <sys/types.h>
#include <libadt/hash_table.h>
#include <as.h>

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#define PLB_GET_CHAR(i)		(tmpfs_reg.plb_ro[(i) % PLB_SIZE])

#define DENTRIES_BUCKETS	256

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

	/*
	 * This is only for debugging. Once we can create files and directories
	 * using VFS, we can get rid of this.
	 */
	tmpfs_dentry_t *d;
	d = (tmpfs_dentry_t *) malloc(sizeof(tmpfs_dentry_t));
	if (!d) {
		free(root);
		root = NULL;
		return false;
	}
	tmpfs_dentry_initialize(d);
	d->index = tmpfs_next_index++;
	root->child = d;
	d->parent = root;
	d->type = TMPFS_DIRECTORY;
	d->name = "dir1";
	hash_table_insert(&dentries, &d->index, &d->dh_link);

	d = (tmpfs_dentry_t *) malloc(sizeof(tmpfs_dentry_t));
	if (!d) {
		free(root->child);
		free(root);
		root = NULL;
		return false;
	}
	tmpfs_dentry_initialize(d);
	d->index = tmpfs_next_index++;
	root->child->sibling = d;
	d->parent = root;
	d->type = TMPFS_DIRECTORY;
	d->name = "dir2";
	hash_table_insert(&dentries, &d->index, &d->dh_link);
	
	d = (tmpfs_dentry_t *) malloc(sizeof(tmpfs_dentry_t));
	if (!d) {
		free(root->child->sibling);
		free(root->child);
		free(root);
		root = NULL;
		return false;
	}
	tmpfs_dentry_initialize(d);
	d->index = tmpfs_next_index++;
	root->child->child = d;
	d->parent = root->child;
	d->type = TMPFS_FILE;
	d->name = "file1";
	d->data = "This is the contents of /dir1/file1.\n";
	d->size = strlen(d->data);
	hash_table_insert(&dentries, &d->index, &d->dh_link);

	d = (tmpfs_dentry_t *) malloc(sizeof(tmpfs_dentry_t));
	if (!d) {
		free(root->child->sibling);
		free(root->child->child);
		free(root->child);
		free(root);
		root = NULL;
		return false;
	}
	tmpfs_dentry_initialize(d);
	d->index = tmpfs_next_index++;
	root->child->sibling->child = d;
	d->parent = root->child->sibling;
	d->type = TMPFS_FILE;
	d->name = "file2";
	d->data = "This is the contents of /dir2/file2.\n";
	d->size = strlen(d->data);
	hash_table_insert(&dentries, &d->index, &d->dh_link);

	return true;
}

/** Compare one component of path to a directory entry.
 *
 * @param dentry	Directory entry to compare the path component with.
 * @param start		Index into PLB where the path component starts.
 * @param last		Index of the last character of the path in PLB.
 *
 * @return		Zero on failure or delta such that (index + delta) %
 *			PLB_SIZE points	to the first unprocessed character in
 *			PLB which comprises the path.
 */
static unsigned match_path_component(tmpfs_dentry_t *dentry, unsigned start,
    unsigned last)
{
	int i, j;
	size_t namelen;

	namelen = strlen(dentry->name);

	if (last < start)
		last += PLB_SIZE;

	for (i = 0, j = start; i < namelen && j <= last; i++, j++) {
		if (dentry->name[i] != PLB_GET_CHAR(j))
			return 0;
	}
	
	if (i != namelen)
		return 0;
	if (j < last && PLB_GET_CHAR(j) != '/')
		return 0;
	if (j == last)
	    	return 0;
	
	return (j - start);
}

void tmpfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	unsigned next = IPC_GET_ARG1(*request);
	unsigned last = IPC_GET_ARG2(*request);
	int dev_handle = IPC_GET_ARG3(*request);

	if (last < next)
		last += PLB_SIZE;

	if (!root && !tmpfs_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	tmpfs_dentry_t *dtmp = root->child; 
	tmpfs_dentry_t *dcur = root;

	bool hit = true;
	
	if (PLB_GET_CHAR(next) == '/')
		next++;		/* eat slash */
	
	while (next <= last) {
		unsigned delta;
		hit = false;
		do {
			delta = match_path_component(dtmp, next, last);
			if (!delta) {
				dtmp = dtmp->sibling;
			} else {
				hit = true;
				next += delta;
				next++;		/* eat slash */
				dcur = dtmp;
				dtmp = dtmp->child;
			}	
		} while (delta == 0 && dtmp);
		if (!hit) {
			ipc_answer_3(rid, ENOENT, tmpfs_reg.fs_handle,
			    dev_handle, dcur->index);
			return;
		}
	}

	ipc_answer_3(rid, EOK, tmpfs_reg.fs_handle, dev_handle, dcur->index);
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

	size_t bytes = max(0, min(dentry->size - pos, len));
	(void) ipc_data_read_deliver(callid, dentry->data + pos, bytes);

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
		(void) ipc_data_write_deliver(callid, dentry->data + pos, len);
		ipc_answer_1(rid, EOK, len);
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
		ipc_answer_1(rid, EOK, 0);
		return;
	}
	dentry->size += delta;
	dentry->data = newdata;
	(void) ipc_data_write_deliver(callid, dentry->data + pos, len);
	ipc_answer_1(rid, EOK, len);
}

/**
 * @}
 */ 
