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
 * @file	vfs_lookup.c
 * @brief	VFS_LOOKUP method and Path Lookup Buffer code.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <bool.h>
#include <futex.h>
#include <libadt/list.h>
#include "vfs.h"

#define min(a, b)	((a) < (b) ? (a) : (b))

atomic_t plb_futex = FUTEX_INITIALIZER;
link_t plb_head;	/**< PLB entry ring buffer. */
uint8_t *plb = NULL;

/** Perform a path lookup.
 *
 * @param path		Path to be resolved; it needn't be an ASCIIZ string.
 * @param len		Number of path characters pointed by path.
 * @param result	Empty node structure where the result will be stored.
 * @param size		Storage where the size of the node will be stored. Can
 *			be NULL.
 * @param altroot	If non-empty, will be used instead of rootfs as the root
 *			of the whole VFS tree.
 *
 * @return		EOK on success or an error code from errno.h.
 */
int vfs_lookup_internal(char *path, size_t len, vfs_triplet_t *result,
    size_t *size, vfs_pair_t *altroot)
{
	vfs_pair_t *root;

	if (!len)
		return EINVAL;

	if (altroot)
		root = altroot;
	else
		root = (vfs_pair_t *) &rootfs;

	if (!root->fs_handle)
		return ENOENT;
	
	futex_down(&plb_futex);

	plb_entry_t entry;
	link_initialize(&entry.plb_link);
	entry.len = len;

	off_t first;	/* the first free index */
	off_t last;	/* the last free index */

	if (list_empty(&plb_head)) {
		first = 0;
		last = PLB_SIZE - 1;
	} else {
		plb_entry_t *oldest = list_get_instance(plb_head.next,
		    plb_entry_t, plb_link);
		plb_entry_t *newest = list_get_instance(plb_head.prev,
		    plb_entry_t, plb_link);

		first = (newest->index + newest->len) % PLB_SIZE;
		last = (oldest->index - 1) % PLB_SIZE;
	}

	if (first <= last) {
		if ((last - first) + 1 < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			futex_up(&plb_futex);
			return ELIMIT;
		}
	} else {
		if (PLB_SIZE - ((first - last) + 1) < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			futex_up(&plb_futex);
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
	list_append(&entry.plb_link, &plb_head);
	
	futex_up(&plb_futex);

	/*
	 * Copy the path into PLB.
	 */
	size_t cnt1 = min(len, (PLB_SIZE - first) + 1);
	size_t cnt2 = len - cnt1;
	
	memcpy(&plb[first], path, cnt1);
	memcpy(plb, &path[cnt1], cnt2);

	ipc_call_t answer;
	int phone = vfs_grab_phone(root->fs_handle);
	aid_t req = async_send_3(phone, VFS_LOOKUP, (ipcarg_t) first,
	    (ipcarg_t) (first + len - 1) % PLB_SIZE,
	    (ipcarg_t) root->dev_handle, &answer);
	vfs_release_phone(phone);

	ipcarg_t rc;
	async_wait_for(req, &rc);

	futex_down(&plb_futex);
	list_remove(&entry.plb_link);
	/*
	 * Erasing the path from PLB will come handy for debugging purposes.
	 */
	memset(&plb[first], 0, cnt1);
	memset(plb, 0, cnt2);
	futex_up(&plb_futex);

	if (rc == EOK) {
		result->fs_handle = (int) IPC_GET_ARG1(answer);
		result->dev_handle = (int) IPC_GET_ARG2(answer);
		result->index = (int) IPC_GET_ARG3(answer);
		if (size)
			*size = (size_t) IPC_GET_ARG4(answer);
	}

	return rc;
}

/**
 * @}
 */ 
