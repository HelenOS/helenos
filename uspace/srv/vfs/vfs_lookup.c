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
 * @file vfs_lookup.c
 * @brief
 */

#include "vfs.h"
#include <macros.h>
#include <async.h>
#include <errno.h>
#include <str.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <vfs/canonify.h>

#define min(a, b)  ((a) < (b) ? (a) : (b))

FIBRIL_MUTEX_INITIALIZE(plb_mutex);
LIST_INITIALIZE(plb_entries);	/**< PLB entry ring buffer. */
uint8_t *plb = NULL;

/** Perform a path lookup.
 *
 * @param path    Path to be resolved; it must be a NULL-terminated
 *                string.
 * @param lflag   Flags to be used during lookup.
 * @param result  Empty structure where the lookup result will be stored.
 *                Can be NULL.
 * @param altroot If non-empty, will be used instead of rootfs as the root
 *                of the whole VFS tree.
 *
 * @return EOK on success or an error code from errno.h.
 *
 */
int vfs_lookup_internal(char *path, int lflag, vfs_lookup_res_t *result,
    vfs_pair_t *altroot, ...)
{
	vfs_pair_t *root;

	if (altroot)
		root = altroot;
	else
		root = &rootfs;

	if (!root->fs_handle)
		return ENOENT;
	
	size_t len;
	path = canonify(path, &len);
	if (!path)
		return EINVAL;
	
	fs_index_t index = 0;
	if (lflag & L_LINK) {
		va_list ap;

		va_start(ap, altroot);
		index = va_arg(ap, fs_index_t);
		va_end(ap);
	}
	
	fibril_mutex_lock(&plb_mutex);

	plb_entry_t entry;
	link_initialize(&entry.plb_link);
	entry.len = len;

	size_t first;	/* the first free index */
	size_t last;	/* the last free index */

	if (list_empty(&plb_entries)) {
		first = 0;
		last = PLB_SIZE - 1;
	} else {
		plb_entry_t *oldest = list_get_instance(
		    list_first(&plb_entries), plb_entry_t, plb_link);
		plb_entry_t *newest = list_get_instance(
		    list_last(&plb_entries), plb_entry_t, plb_link);

		first = (newest->index + newest->len) % PLB_SIZE;
		last = (oldest->index - 1) % PLB_SIZE;
	}

	if (first <= last) {
		if ((last - first) + 1 < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			fibril_mutex_unlock(&plb_mutex);
			return ELIMIT;
		}
	} else {
		if (PLB_SIZE - ((first - last) + 1) < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			fibril_mutex_unlock(&plb_mutex);
			return ELIMIT;
		}
	}

	/*
	 * We know the first free index in PLB and we also know that there is
	 * enough space in the buffer to hold our path.
	 */

	entry.index = first;
	entry.len = len;

	/*
	 * Claim PLB space by inserting the entry into the PLB entry ring
	 * buffer.
	 */
	list_append(&entry.plb_link, &plb_entries);
	
	fibril_mutex_unlock(&plb_mutex);

	/*
	 * Copy the path into PLB.
	 */
	size_t cnt1 = min(len, (PLB_SIZE - first) + 1);
	size_t cnt2 = len - cnt1;
	
	memcpy(&plb[first], path, cnt1);
	memcpy(plb, &path[cnt1], cnt2);

	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_grab(root->fs_handle);
	aid_t req = async_send_5(exch, VFS_OUT_LOOKUP, (sysarg_t) first,
	    (sysarg_t) (first + len - 1) % PLB_SIZE,
	    (sysarg_t) root->service_id, (sysarg_t) lflag, (sysarg_t) index,
	    &answer);
	
	sysarg_t rc;
	async_wait_for(req, &rc);
	vfs_exchange_release(exch);
	
	fibril_mutex_lock(&plb_mutex);
	list_remove(&entry.plb_link);
	/*
	 * Erasing the path from PLB will come handy for debugging purposes.
	 */
	memset(&plb[first], 0, cnt1);
	memset(plb, 0, cnt2);
	fibril_mutex_unlock(&plb_mutex);
	
	if ((int) rc < EOK)
		return (int) rc;

	if (!result)
		return EOK;
	
	result->triplet.fs_handle = (fs_handle_t) rc;
	result->triplet.service_id = (service_id_t) IPC_GET_ARG1(answer);
	result->triplet.index = (fs_index_t) IPC_GET_ARG2(answer);
	result->size =
	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(answer), IPC_GET_ARG4(answer));
	result->lnkcnt = (unsigned int) IPC_GET_ARG5(answer);
	
	if (lflag & L_FILE)
		result->type = VFS_NODE_FILE;
	else if (lflag & L_DIRECTORY)
		result->type = VFS_NODE_DIRECTORY;
	else
		result->type = VFS_NODE_UNKNOWN;
	
	return EOK;
}

/**
 * @}
 */
