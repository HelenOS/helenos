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
 * @file	vfs_mount.c
 * @brief	VFS_MOUNT method.
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

atomic_t rootfs_futex = FUTEX_INITIALIZER;
vfs_triplet_t rootfs = {
	.fs_handle = 0,
	.dev_handle = 0,
	.index = 0,
};

static int lookup_root(int fs_handle, int dev_handle, vfs_triplet_t *root)
{
	vfs_pair_t altroot = {
		.fs_handle = fs_handle,
		.dev_handle = dev_handle,
	};

	return vfs_lookup_internal("/", strlen("/"), root, &altroot);
}

void vfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	int dev_handle;
	vfs_node_t *mp_node = NULL;

	/*
	 * We expect the library to do the device-name to device-handle
	 * translation for us, thus the device handle will arrive as ARG1
	 * in the request.
	 */
	dev_handle = IPC_GET_ARG1(*request);

	/*
	 * For now, don't make use of ARG2 and ARG3, but they can be used to
	 * carry mount options in the future.
	 */

	/*
	 * Now, we expect the client to send us data with the name of the file
	 * system and the path of the mountpoint.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_receive(&callid, NULL, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * There is no sense in receiving data that can't hold a single
	 * character of path. We won't accept data that exceed certain limits
	 * either.
	 */
	if ((size < FS_NAME_MAXLEN + 1) ||
	    (size > FS_NAME_MAXLEN + MAX_PATH_LEN)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Allocate buffer for the data being received.
	 */
	uint8_t *buf;
	buf = malloc(size);
	if (!buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Deliver the data.
	 */
	(void) ipc_data_deliver(callid, buf, size);

	char fs_name[FS_NAME_MAXLEN + 1];
	memcpy(fs_name, buf, FS_NAME_MAXLEN);
	fs_name[FS_NAME_MAXLEN] = '\0';

	/*
	 * Check if we know a file system with the same name as is in fs_name.
	 * This will also give us its file system handle.
	 */
	int fs_handle = fs_name_to_handle(fs_name, true);
	if (!fs_handle) {
		free(buf);
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/*
	 * Lookup the root node of the filesystem being mounted.
	 * In this case, we don't need to take the unlink_futex as the root node
	 * cannot be removed. However, we do take a reference to it so that
	 * we can track how many times it has been mounted.
	 */
	int rc;
	vfs_triplet_t mounted_root;
	rc = lookup_root(fs_handle, dev_handle, &mounted_root);
	if (rc != EOK) {
		free(buf);
		ipc_answer_0(rid, rc);
		return;
	}
	vfs_node_t *mr_node = vfs_node_get(&mounted_root);
	if (!mr_node) {
		free(buf);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Finally, we need to resolve the path to the mountpoint. 
	 */
	vfs_triplet_t mp;
	futex_down(&rootfs_futex);
	if (rootfs.fs_handle) {
		/*
		 * We already have the root FS.
		 */
		futex_down(&unlink_futex);
		rc = vfs_lookup_internal((char *) (buf + FS_NAME_MAXLEN),
		    size - FS_NAME_MAXLEN, &mp, NULL);
		if (rc != EOK) {
			/*
			 * The lookup failed for some reason.
			 */
			futex_up(&unlink_futex);
			futex_up(&rootfs_futex);
			vfs_node_put(mr_node);	/* failed -> drop reference */
			free(buf);
			ipc_answer_0(rid, rc);
			return;
		}
		mp_node = vfs_node_get(&mp);
		if (!mp_node) {
			futex_up(&unlink_futex);
			futex_up(&rootfs_futex);
			vfs_node_put(mr_node);	/* failed -> drop reference */
			free(buf);
			ipc_answer_0(rid, ENOMEM);
			return;
		}
		/*
		 * Now we hold a reference to mp_node.
		 * It will be dropped upon the corresponding VFS_UNMOUNT.
		 * This prevents the mount point from being deleted.
		 */
		futex_up(&unlink_futex);
	} else {
		/*
		 * We still don't have the root file system mounted.
		 */
		if ((size - FS_NAME_MAXLEN == strlen("/")) &&
		    (buf[FS_NAME_MAXLEN] == '/')) {
			/*
			 * For this simple, but important case, we are done.
			 */
			rootfs = mounted_root;
			futex_up(&rootfs_futex);
			free(buf);
			ipc_answer_0(rid, EOK);
			return;
		} else {
			/*
			 * We can't resolve this without the root filesystem
			 * being mounted first.
			 */
			futex_up(&rootfs_futex);
			free(buf);
			vfs_node_put(mr_node);	/* failed -> drop reference */
			ipc_answer_0(rid, ENOENT);
			return;
		}
	}
	futex_up(&rootfs_futex);
	
	free(buf);	/* The buffer is not needed anymore. */
	
	/*
	 * At this point, we have all necessary pieces: file system and device
	 * handles, and we know the mount point VFS node and also the root node
	 * of the file system being mounted.
	 */

	int phone = vfs_grab_phone(mp.fs_handle);
	/* Later we can use ARG3 to pass mode/flags. */
	aid_t req1 = async_send_3(phone, VFS_MOUNT, (ipcarg_t) mp.dev_handle,
	    (ipcarg_t) mp.index, 0, NULL);
	/* The second call uses the same method. */
	aid_t req2 = async_send_3(phone, VFS_MOUNT,
	    (ipcarg_t) mounted_root.fs_handle,
	    (ipcarg_t) mounted_root.dev_handle, (ipcarg_t) mounted_root.index,
	    NULL);

	ipcarg_t rc1;
	ipcarg_t rc2;
	async_wait_for(req1, &rc1);
	async_wait_for(req2, &rc2);
	vfs_release_phone(phone);

	if ((rc1 != EOK) || (rc2 != EOK)) {
		/* Mount failed, drop references to mr_node and mp_node. */
		vfs_node_put(mr_node);
		if (mp_node)
			vfs_node_put(mp_node);
	}
	
	if (rc2 == EOK)
		ipc_answer_0(rid, rc1);
	else if (rc1 == EOK)
		ipc_answer_0(rid, rc2);
	else
		ipc_answer_0(rid, rc1);
}

/**
 * @}
 */ 
