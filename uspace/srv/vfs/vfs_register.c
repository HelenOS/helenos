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
 * @file vfs_register.c
 * @brief
 */

#include <ipc/services.h>
#include <async.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <stdbool.h>
#include <adt/list.h>
#include <as.h>
#include <assert.h>
#include <atomic.h>
#include <vfs/vfs.h>
#include "vfs.h"

FIBRIL_CONDVAR_INITIALIZE(fs_list_cv);
FIBRIL_MUTEX_INITIALIZE(fs_list_lock);
LIST_INITIALIZE(fs_list);

atomic_t fs_handle_next = {
	.count = 1
};

/** Verify the VFS info structure.
 *
 * @param info Info structure to be verified.
 *
 * @return Non-zero if the info structure is sane, zero otherwise.
 *
 */
static bool vfs_info_sane(vfs_info_t *info)
{
	int i;

	/*
	 * Check if the name is non-empty and is composed solely of ASCII
	 * characters [a-z]+[a-z0-9_-]*.
	 */
	if (!islower(info->name[0])) {
		dprintf("The name doesn't start with a lowercase character.\n");
		return false;
	}

	for (i = 1; i < FS_NAME_MAXLEN; i++) {
		if (!(islower(info->name[i]) || isdigit(info->name[i])) &&
		    (info->name[i] != '-') && (info->name[i] != '_')) {
			if (info->name[i] == '\0') {
				break;
			} else {
				dprintf("The name contains illegal "
				    "characters.\n");
				return false;
			}
		}
	}

	/*
	 * This check is not redundant. It ensures that the name is
	 * NULL-terminated, even if FS_NAME_MAXLEN characters are used.
	 */
	if (info->name[i] != '\0') {
		dprintf("The name is not properly NULL-terminated.\n");
		return false;
	}

	return true;
}

/** VFS_REGISTER protocol function.
 *
 * @param rid     Hash of the call with the request.
 * @param request Call structure with the request.
 *
 */
void vfs_register(ipc_callid_t rid, ipc_call_t *request)
{
	dprintf("Processing VFS_REGISTER request received from %zx.\n",
	    request->in_phone_hash);

	vfs_info_t *vfs_info;
	errno_t rc = async_data_write_accept((void **) &vfs_info, false,
	    sizeof(vfs_info_t), sizeof(vfs_info_t), 0, NULL);

	if (rc != EOK) {
		dprintf("Failed to deliver the VFS info into our AS, rc=%d.\n",
		    rc);
		async_answer_0(rid, rc);
		return;
	}

	/*
	 * Allocate and initialize a buffer for the fs_info structure.
	 */
	fs_info_t *fs_info = (fs_info_t *) malloc(sizeof(fs_info_t));
	if (!fs_info) {
		dprintf("Could not allocate memory for FS info.\n");
		async_answer_0(rid, ENOMEM);
		return;
	}

	link_initialize(&fs_info->fs_link);
	fs_info->vfs_info = *vfs_info;
	free(vfs_info);

	dprintf("VFS info delivered.\n");

	if (!vfs_info_sane(&fs_info->vfs_info)) {
		free(fs_info);
		async_answer_0(rid, EINVAL);
		return;
	}

	fibril_mutex_lock(&fs_list_lock);

	/*
	 * Check for duplicit registrations.
	 */
	if (fs_name_to_handle(fs_info->vfs_info.instance,
	    fs_info->vfs_info.name, false)) {
		/*
		 * We already register a fs like this.
		 */
		dprintf("FS is already registered.\n");
		fibril_mutex_unlock(&fs_list_lock);
		free(fs_info);
		async_answer_0(rid, EEXIST);
		return;
	}

	/*
	 * Add fs_info to the list of registered FS's.
	 */
	dprintf("Inserting FS into the list of registered file systems.\n");
	list_append(&fs_info->fs_link, &fs_list);

	/*
	 * Now we want the client to send us the IPC_M_CONNECT_TO_ME call so
	 * that a callback connection is created and we have a phone through
	 * which to forward VFS requests to it.
	 */
	fs_info->sess = async_callback_receive(EXCHANGE_PARALLEL);
	if (!fs_info->sess) {
		dprintf("Callback connection expected\n");
		list_remove(&fs_info->fs_link);
		fibril_mutex_unlock(&fs_list_lock);
		free(fs_info);
		async_answer_0(rid, EINVAL);
		return;
	}

	dprintf("Callback connection to FS created.\n");

	/*
	 * The client will want us to send him the address space area with PLB.
	 */

	size_t size;
	ipc_callid_t callid;
	if (!async_share_in_receive(&callid, &size)) {
		dprintf("Unexpected call\n");
		list_remove(&fs_info->fs_link);
		fibril_mutex_unlock(&fs_list_lock);
		async_hangup(fs_info->sess);
		free(fs_info);
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * We can only send the client address space area PLB_SIZE bytes long.
	 */
	if (size != PLB_SIZE) {
		dprintf("Client suggests wrong size of PFB, size = %zu\n", size);
		list_remove(&fs_info->fs_link);
		fibril_mutex_unlock(&fs_list_lock);
		async_hangup(fs_info->sess);
		free(fs_info);
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Commit to read-only sharing the PLB with the client.
	 */
	(void) async_share_in_finalize(callid, plb,
	    AS_AREA_READ | AS_AREA_CACHEABLE);

	dprintf("Sharing PLB.\n");

	/*
	 * That was it. The FS has been registered.
	 * In reply to the VFS_REGISTER request, we assign the client file
	 * system a global file system handle.
	 */
	fs_info->fs_handle = (fs_handle_t) atomic_postinc(&fs_handle_next);
	async_answer_1(rid, EOK, (sysarg_t) fs_info->fs_handle);

	fibril_condvar_broadcast(&fs_list_cv);
	fibril_mutex_unlock(&fs_list_lock);

	dprintf("\"%.*s\" filesystem successfully registered, handle=%d.\n",
	    FS_NAME_MAXLEN, fs_info->vfs_info.name, fs_info->fs_handle);
}

/** Begin an exchange for a given file system handle
 *
 * @param handle File system handle.
 *
 * @return Exchange for a multi-call request.
 * @return NULL if no such file exists.
 *
 */
async_exch_t *vfs_exchange_grab(fs_handle_t handle)
{
	/*
	 * For now, we don't try to be very clever and very fast.
	 * We simply lookup the session in fs_list and
	 * begin an exchange.
	 */
	fibril_mutex_lock(&fs_list_lock);

	list_foreach(fs_list, fs_link, fs_info_t, fs) {
		if (fs->fs_handle == handle) {
			fibril_mutex_unlock(&fs_list_lock);

			assert(fs->sess);
			async_exch_t *exch = async_exchange_begin(fs->sess);

			assert(exch);
			return exch;
		}
	}

	fibril_mutex_unlock(&fs_list_lock);

	return NULL;
}

/** End VFS server exchange.
 *
 * @param exch   VFS server exchange.
 *
 */
void vfs_exchange_release(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Convert file system name to its handle.
 *
 * @param name File system name.
 * @param lock If true, the function will lock and unlock the
 *             fs_list_lock.
 *
 * @return File system handle or zero if file system not found.
 *
 */
fs_handle_t fs_name_to_handle(unsigned int instance, const char *name, bool lock)
{
	int handle = 0;

	if (lock)
		fibril_mutex_lock(&fs_list_lock);

	list_foreach(fs_list, fs_link, fs_info_t, fs) {
		if (str_cmp(fs->vfs_info.name, name) == 0 &&
		    instance == fs->vfs_info.instance) {
			handle = fs->fs_handle;
			break;
		}
	}

	if (lock)
		fibril_mutex_unlock(&fs_list_lock);

	return handle;
}

/** Find the VFS info structure.
 *
 * @param handle FS handle for which the VFS info structure is sought.
 *
 * @return VFS info structure on success or NULL otherwise.
 *
 */
vfs_info_t *fs_handle_to_info(fs_handle_t handle)
{
	vfs_info_t *info = NULL;

	fibril_mutex_lock(&fs_list_lock);
	list_foreach(fs_list, fs_link, fs_info_t, fs) {
		if (fs->fs_handle == handle) {
			info = &fs->vfs_info;
			break;
		}
	}
	fibril_mutex_unlock(&fs_list_lock);

	return info;
}

/** Get list of file system types.
 *
 * @param fstypes Place to store list of file system types. Free using
 *                vfs_fstypes_free().
 *
 * @return EOK on success or an error code
 */
errno_t vfs_get_fstypes(vfs_fstypes_t *fstypes)
{
	size_t size;
	size_t count;
	size_t l;

	fibril_mutex_lock(&fs_list_lock);

	size = 0;
	count = 0;
	list_foreach(fs_list, fs_link, fs_info_t, fs) {
		size += str_size(fs->vfs_info.name) + 1;
		count++;
	}

	if (size == 0)
		size = 1;

	fstypes->buf = calloc(1, size);
	if (fstypes->buf == NULL) {
		fibril_mutex_unlock(&fs_list_lock);
		return ENOMEM;
	}

	fstypes->fstypes = calloc(sizeof(char *), count);
	if (fstypes->fstypes == NULL) {
		free(fstypes->buf);
		fstypes->buf = NULL;
		fibril_mutex_unlock(&fs_list_lock);
		return ENOMEM;
	}

	fstypes->size = size;

	size = 0; count = 0;
	list_foreach(fs_list, fs_link, fs_info_t, fs) {
		l = str_size(fs->vfs_info.name) + 1;
		memcpy(fstypes->buf + size, fs->vfs_info.name, l);
		fstypes->fstypes[count] = &fstypes->buf[size];
		size += l;
		count++;
	}

	fibril_mutex_unlock(&fs_list_lock);
	return EOK;
}

/**
 * @}
 */
