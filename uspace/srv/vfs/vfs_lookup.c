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
#include <dirent.h>
#include <assert.h>

#define DPRINTF(...)

#define min(a, b)  ((a) < (b) ? (a) : (b))

FIBRIL_MUTEX_INITIALIZE(plb_mutex);
LIST_INITIALIZE(plb_entries);	/**< PLB entry ring buffer. */
uint8_t *plb = NULL;

static int plb_insert_entry(plb_entry_t *entry, char *path, size_t *start, size_t len)
{
	fibril_mutex_lock(&plb_mutex);

	link_initialize(&entry->plb_link);
	entry->len = len;

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

	entry->index = first;
	entry->len = len;

	/*
	 * Claim PLB space by inserting the entry into the PLB entry ring
	 * buffer.
	 */
	list_append(&entry->plb_link, &plb_entries);
	
	fibril_mutex_unlock(&plb_mutex);

	/*
	 * Copy the path into PLB.
	 */
	size_t cnt1 = min(len, (PLB_SIZE - first) + 1);
	size_t cnt2 = len - cnt1;
	
	memcpy(&plb[first], path, cnt1);
	memcpy(plb, &path[cnt1], cnt2);

	*start = first;
	return EOK;
}

static void plb_clear_entry(plb_entry_t *entry, size_t first, size_t len)
{
	fibril_mutex_lock(&plb_mutex);
	list_remove(&entry->plb_link);
	/*
	 * Erasing the path from PLB will come handy for debugging purposes.
	 */
	size_t cnt1 = min(len, (PLB_SIZE - first) + 1);
	size_t cnt2 = len - cnt1;
	memset(&plb[first], 0, cnt1);
	memset(plb, 0, cnt2);
	fibril_mutex_unlock(&plb_mutex);
}

static char *_strrchr(char *path, int c)
{
	char *res = NULL;
	while (*path != 0) {
		if (*path == c) {
			res = path;
		}
		path++;
	}
	return res;
}

int vfs_link_internal(vfs_triplet_t *base, char *path, vfs_triplet_t *child)
{
	assert(base != NULL);
	assert(child != NULL);
	assert(base->fs_handle);
	assert(child->fs_handle);
	assert(path != NULL);
	
	vfs_lookup_res_t res;
	char component[NAME_MAX + 1];
	int rc;
	
	size_t len;
	char *npath = canonify(path, &len);
	if (!npath) {
		rc = EINVAL;
		goto out;
	}
	path = npath;
	
	char *slash = _strrchr(path, '/');
	if (slash && slash != path) {
		if (slash[1] == 0) {
			rc = EINVAL;
			goto out;
		}
		
		memcpy(component, slash + 1, str_size(slash));
		*slash = 0;
		
		rc = vfs_lookup_internal(base, path, L_DIRECTORY, &res);
		if (rc != EOK) {
			goto out;
		}
		base = &res.triplet;
		
		*slash = '/';
	} else {
		memcpy(component, path + 1, str_size(path));
	}
	
	if (base->fs_handle != child->fs_handle || base->service_id != child->service_id) {
		rc = EXDEV;
		goto out;
	}
	
	async_exch_t *exch = vfs_exchange_grab(base->fs_handle);
	aid_t req = async_send_3(exch, VFS_OUT_LINK, base->service_id, base->index, child->index, NULL);
	
	rc = async_data_write_start(exch, component, str_size(component) + 1);
	sysarg_t orig_rc;
	async_wait_for(req, &orig_rc);
	vfs_exchange_release(exch);
	if (orig_rc != EOK) {
		rc = orig_rc;
	}
	
out:
	DPRINTF("vfs_link_internal() with path '%s' returns %d\n", path, rc);
	return rc;
}

/** Perform a path lookup.
 *
 * @param base    The file from which to perform the lookup.
 * @param path    Path to be resolved; it must be a NULL-terminated
 *                string.
 * @param lflag   Flags to be used during lookup.
 * @param result  Empty structure where the lookup result will be stored.
 *                Can be NULL.
 *
 * @return EOK on success or an error code from errno.h.
 *
 */
int vfs_lookup_internal(vfs_triplet_t *base, char *path, int lflag, vfs_lookup_res_t *result)
{
	assert(base != NULL);
	assert(path != NULL);
	
	sysarg_t rc;
	
	if (!base->fs_handle) {
		rc = ENOENT;
		goto out;
	}
	
	size_t len;
	char *npath = canonify(path, &len);
	if (!npath) {
		rc = EINVAL;
		goto out;
	}
	path = npath;
	
	assert(path[0] == '/');
	
	size_t first;
	
	plb_entry_t entry;
	rc = plb_insert_entry(&entry, path, &first, len);
	if (rc != EOK) {
		goto out;
	}
	
	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_grab(base->fs_handle);
	aid_t req = async_send_5(exch, VFS_OUT_LOOKUP, (sysarg_t) first, (sysarg_t) len,
	    (sysarg_t) base->service_id, (sysarg_t) base->index, (sysarg_t) lflag, &answer);
	async_wait_for(req, &rc);
	vfs_exchange_release(exch);
	
	plb_clear_entry(&entry, first, len);
	
	if ((int) rc < 0) {
		goto out;
	}
	
	unsigned last = IPC_GET_ARG3(answer);
	if (last != first + len) {
		/* The path wasn't processed entirely. */
		rc = ENOENT;
		goto out;
	}
	
	if (!result) {
		rc = EOK;
		goto out;
	}
	
	result->triplet.fs_handle = (fs_handle_t) rc;
	result->triplet.service_id = (service_id_t) IPC_GET_ARG1(answer);
	result->triplet.index = (fs_index_t) IPC_GET_ARG2(answer);
	result->size = (int64_t)(int32_t) IPC_GET_ARG4(answer);
	result->type = IPC_GET_ARG5(answer);
	rc = EOK;

out:
	DPRINTF("vfs_lookup_internal() with path '%s' returns %d\n", path, rc);
	return rc;
}

/**
 * @}
 */
