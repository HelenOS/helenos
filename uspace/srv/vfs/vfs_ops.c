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
 * @file	vfs_ops.c
 * @brief	Operations that VFS offers to its clients.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bool.h>
#include <futex.h>
#include <rwlock.h>
#include <libadt/list.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <atomic.h>
#include "vfs.h"

/* Forward declarations of static functions. */
static int vfs_truncate_internal(int, int, unsigned long, size_t);

/**
 * This rwlock prevents the race between a triplet-to-VFS-node resolution and a
 * concurrent VFS operation which modifies the file system namespace.
 */
RWLOCK_INITIALIZE(namespace_rwlock);

atomic_t rootfs_futex = FUTEX_INITIALIZER;
vfs_triplet_t rootfs = {
	.fs_handle = 0,
	.dev_handle = 0,
	.index = 0,
};

static int lookup_root(int fs_handle, int dev_handle, vfs_lookup_res_t *result)
{
	vfs_pair_t altroot = {
		.fs_handle = fs_handle,
		.dev_handle = dev_handle,
	};

	return vfs_lookup_internal("/", strlen("/"), L_DIRECTORY, result,
	    &altroot);
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

	ipc_callid_t callid;
	size_t size;

	/*
	 * Now, we expect the client to send us data with the name of the file
	 * system.
	 */
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Don't receive more than is necessary for storing a full file system
	 * name.
	 */
	if (size < 1 || size > FS_NAME_MAXLEN) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/* Deliver the file system name. */
	char fs_name[FS_NAME_MAXLEN + 1];
	(void) ipc_data_write_finalize(callid, fs_name, size);
	fs_name[size] = '\0';
	
	/*
	 * Check if we know a file system with the same name as is in fs_name.
	 * This will also give us its file system handle.
	 */
	int fs_handle = fs_name_to_handle(fs_name, true);
	if (!fs_handle) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/* Now, we want the client to send us the mount point. */
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/* Check whether size is reasonable wrt. the mount point. */
	if (size < 1 || size > MAX_PATH_LEN) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	/* Allocate buffer for the mount point data being received. */
	uint8_t *buf;
	buf = malloc(size);
	if (!buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/* Deliver the mount point. */
	(void) ipc_data_write_finalize(callid, buf, size);

	/*
	 * Lookup the root node of the filesystem being mounted.
	 * In this case, we don't need to take the namespace_futex as the root
	 * node cannot be removed. However, we do take a reference to it so
	 * that we can track how many times it has been mounted.
	 */
	int rc;
	vfs_lookup_res_t mr_res;
	rc = lookup_root(fs_handle, dev_handle, &mr_res);
	if (rc != EOK) {
		free(buf);
		ipc_answer_0(rid, rc);
		return;
	}
	vfs_node_t *mr_node = vfs_node_get(&mr_res);
	if (!mr_node) {
		free(buf);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/* Finally, we need to resolve the path to the mountpoint. */
	vfs_lookup_res_t mp_res;
	futex_down(&rootfs_futex);
	if (rootfs.fs_handle) {
		/* We already have the root FS. */
		rwlock_write_lock(&namespace_rwlock);
		rc = vfs_lookup_internal(buf, size, L_DIRECTORY, &mp_res,
		    NULL);
		if (rc != EOK) {
			/* The lookup failed for some reason. */
			rwlock_write_unlock(&namespace_rwlock);
			futex_up(&rootfs_futex);
			vfs_node_put(mr_node);	/* failed -> drop reference */
			free(buf);
			ipc_answer_0(rid, rc);
			return;
		}
		mp_node = vfs_node_get(&mp_res);
		if (!mp_node) {
			rwlock_write_unlock(&namespace_rwlock);
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
		rwlock_write_unlock(&namespace_rwlock);
	} else {
		/* We still don't have the root file system mounted. */
		if ((size == 1) && (buf[0] == '/')) {
			/* For this simple, but important case, we are done. */
			rootfs = mr_res.triplet;
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

	int phone = vfs_grab_phone(mp_res.triplet.fs_handle);
	/* Later we can use ARG3 to pass mode/flags. */
	aid_t req1 = async_send_3(phone, VFS_MOUNT,
	    (ipcarg_t) mp_res.triplet.dev_handle,
	    (ipcarg_t) mp_res.triplet.index, 0, NULL);
	/* The second call uses the same method. */
	aid_t req2 = async_send_3(phone, VFS_MOUNT,
	    (ipcarg_t) mr_res.triplet.fs_handle,
	    (ipcarg_t) mr_res.triplet.dev_handle,
	    (ipcarg_t) mr_res.triplet.index, NULL);
	vfs_release_phone(phone);

	ipcarg_t rc1;
	ipcarg_t rc2;
	async_wait_for(req1, &rc1);
	async_wait_for(req2, &rc2);

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

void vfs_open(ipc_callid_t rid, ipc_call_t *request)
{
	if (!vfs_files_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * The POSIX interface is open(path, oflag, mode).
	 * We can receive oflags and mode along with the VFS_OPEN call; the path
	 * will need to arrive in another call.
	 *
	 * We also receive one private, non-POSIX set of flags called lflag
	 * used to pass information to vfs_lookup_internal().
	 */
	int lflag = IPC_GET_ARG1(*request);
	int oflag = IPC_GET_ARG2(*request);
	int mode = IPC_GET_ARG3(*request);
	size_t len;

	if (oflag & O_CREAT)
		lflag |= L_CREATE;
	if (oflag & O_EXCL)
		lflag |= L_EXCLUSIVE;

	ipc_callid_t callid;

	if (!ipc_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Now we are on the verge of accepting the path.
	 *
	 * There is one optimization we could do in the future: copy the path
	 * directly into the PLB using some kind of a callback.
	 */
	char *path = malloc(len);
	
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	int rc;
	if ((rc = ipc_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	
	/*
	 * Avoid the race condition in which the file can be deleted before we
	 * find/create-and-lock the VFS node corresponding to the looked-up
	 * triplet.
	 */
	if (lflag & L_CREATE)
		rwlock_write_lock(&namespace_rwlock);
	else
		rwlock_read_lock(&namespace_rwlock);

	/* The path is now populated and we can call vfs_lookup_internal(). */
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(path, len, lflag, &lr, NULL);
	if (rc) {
		if (lflag & L_CREATE)
			rwlock_write_unlock(&namespace_rwlock);
		else
			rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}

	/* Path is no longer needed. */
	free(path);

	vfs_node_t *node = vfs_node_get(&lr);
	if (lflag & L_CREATE)
		rwlock_write_unlock(&namespace_rwlock);
	else
		rwlock_read_unlock(&namespace_rwlock);

	/* Truncate the file if requested and if necessary. */
	if (oflag & O_TRUNC) {
		futex_down(&node->contents_rwlock);
		if (node->size) {
			rc = vfs_truncate_internal(node->fs_handle,
			    node->dev_handle, node->index, 0);
			if (rc) {
				futex_up(&node->contents_rwlock);
				vfs_node_put(node);
				ipc_answer_0(rid, rc);
				return;
			}
			node->size = 0;
		}
		futex_up(&node->contents_rwlock);
	}

	/*
	 * Get ourselves a file descriptor and the corresponding vfs_file_t
	 * structure.
	 */
	int fd = vfs_fd_alloc();
	if (fd < 0) {
		vfs_node_put(node);
		ipc_answer_0(rid, fd);
		return;
	}
	vfs_file_t *file = vfs_file_get(fd);
	file->node = node;
	if (oflag & O_APPEND) 
		file->append = true;

	/*
	 * The following increase in reference count is for the fact that the
	 * file is being opened and that a file structure is pointing to it.
	 * It is necessary so that the file will not disappear when
	 * vfs_node_put() is called. The reference will be dropped by the
	 * respective VFS_CLOSE.
	 */
	vfs_node_addref(node);
	vfs_node_put(node);

	/* Success! Return the new file descriptor to the client. */
	ipc_answer_1(rid, EOK, fd);
}

void vfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	if (fd >= MAX_OPEN_FILES) {
		ipc_answer_0(rid, EBADF);
		return;
	}
	vfs_fd_free(fd);
	ipc_answer_0(rid, EOK);
}

static void vfs_rdwr(ipc_callid_t rid, ipc_call_t *request, bool read)
{

	/*
	 * The following code strongly depends on the fact that the files data
	 * structure can be only accessed by a single fibril and all file
	 * operations are serialized (i.e. the reads and writes cannot
	 * interleave and a file cannot be closed while it is being read).
	 *
	 * Additional synchronization needs to be added once the table of
	 * open files supports parallel access!
	 */

	int fd = IPC_GET_ARG1(*request);

	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/*
	 * Now we need to receive a call with client's
	 * IPC_M_DATA_READ/IPC_M_DATA_WRITE request.
	 */
	ipc_callid_t callid;
	int res;
	if (read)
		res = ipc_data_read_receive(&callid, NULL);
	else 
		res = ipc_data_write_receive(&callid, NULL);
	if (!res) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	futex_down(&file->lock);

	/*
	 * Lock the file's node so that no other client can read/write to it at
	 * the same time.
	 */
	if (read)
		rwlock_read_lock(&file->node->contents_rwlock);
	else
		rwlock_write_lock(&file->node->contents_rwlock);

	int fs_phone = vfs_grab_phone(file->node->fs_handle);	
	
	/* Make a VFS_READ/VFS_WRITE request at the destination FS server. */
	aid_t msg;
	ipc_call_t answer;
	if (!read && file->append)
		file->pos = file->node->size;
	msg = async_send_3(fs_phone, IPC_GET_METHOD(*request),
	    file->node->dev_handle, file->node->index, file->pos, &answer);
	
	/*
	 * Forward the IPC_M_DATA_READ/IPC_M_DATA_WRITE request to the
	 * destination FS server. The call will be routed as if sent by
	 * ourselves. Note that call arguments are immutable in this case so we
	 * don't have to bother.
	 */
	ipc_forward_fast(callid, fs_phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);

	vfs_release_phone(fs_phone);

	/* Wait for reply from the FS server. */
	ipcarg_t rc;
	async_wait_for(msg, &rc);
	size_t bytes = IPC_GET_ARG1(answer);

	/* Unlock the VFS node. */
	if (read)
		rwlock_read_unlock(&file->node->contents_rwlock);
	else {
		/* Update the cached version of node's size. */
		if (rc == EOK)
			file->node->size = IPC_GET_ARG2(answer); 
		rwlock_write_unlock(&file->node->contents_rwlock);
	}

	/* Update the position pointer and unlock the open file. */
	if (rc == EOK)
		file->pos += bytes;
	futex_up(&file->lock);

	/*
	 * FS server's reply is the final result of the whole operation we
	 * return to the client.
	 */
	ipc_answer_1(rid, rc, bytes);
}

void vfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	vfs_rdwr(rid, request, true);
}

void vfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	vfs_rdwr(rid, request, false);
}

void vfs_seek(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = (int) IPC_GET_ARG1(*request);
	off_t off = (off_t) IPC_GET_ARG2(*request);
	int whence = (int) IPC_GET_ARG3(*request);


	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	off_t newpos;
	futex_down(&file->lock);
	if (whence == SEEK_SET) {
		file->pos = off;
		futex_up(&file->lock);
		ipc_answer_1(rid, EOK, off);
		return;
	}
	if (whence == SEEK_CUR) {
		if (file->pos + off < file->pos) {
			futex_up(&file->lock);
			ipc_answer_0(rid, EOVERFLOW);
			return;
		}
		file->pos += off;
		newpos = file->pos;
		futex_up(&file->lock);
		ipc_answer_1(rid, EOK, newpos);
		return;
	}
	if (whence == SEEK_END) {
		rwlock_read_lock(&file->node->contents_rwlock);
		size_t size = file->node->size;
		rwlock_read_unlock(&file->node->contents_rwlock);
		if (size + off < size) {
			futex_up(&file->lock);
			ipc_answer_0(rid, EOVERFLOW);
			return;
		}
		newpos = size + off;
		futex_up(&file->lock);
		ipc_answer_1(rid, EOK, newpos);
		return;
	}
	futex_up(&file->lock);
	ipc_answer_0(rid, EINVAL);
}

int vfs_truncate_internal(int fs_handle, int dev_handle, unsigned long index,
    size_t size)
{
	ipcarg_t rc;
	int fs_phone;
	
	fs_phone = vfs_grab_phone(fs_handle);
	rc = async_req_3_0(fs_phone, VFS_TRUNCATE, (ipcarg_t)dev_handle,
	    (ipcarg_t)index, (ipcarg_t)size);
	vfs_release_phone(fs_phone);
	return (int)rc;
}

void vfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	size_t size = IPC_GET_ARG2(*request);
	int rc;

	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	futex_down(&file->lock);

	rwlock_write_lock(&file->node->contents_rwlock);
	rc = vfs_truncate_internal(file->node->fs_handle,
	    file->node->dev_handle, file->node->index, size);
	if (rc == EOK)
		file->node->size = size;
	rwlock_write_unlock(&file->node->contents_rwlock);

	futex_up(&file->lock);
	ipc_answer_0(rid, (ipcarg_t)rc);
}

void vfs_mkdir(ipc_callid_t rid, ipc_call_t *request)
{
	int mode = IPC_GET_ARG1(*request);

	size_t len;
	ipc_callid_t callid;

	if (!ipc_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Now we are on the verge of accepting the path.
	 *
	 * There is one optimization we could do in the future: copy the path
	 * directly into the PLB using some kind of a callback.
	 */
	char *path = malloc(len);
	
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	int rc;
	if ((rc = ipc_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	
	rwlock_write_lock(&namespace_rwlock);
	int lflag = L_DIRECTORY | L_CREATE | L_EXCLUSIVE;
	rc = vfs_lookup_internal(path, len, lflag, NULL, NULL);
	rwlock_write_unlock(&namespace_rwlock);
	free(path);
	ipc_answer_0(rid, rc);
}

void vfs_unlink(ipc_callid_t rid, ipc_call_t *request)
{
	int lflag = IPC_GET_ARG1(*request);

	size_t len;
	ipc_callid_t callid;

	if (!ipc_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Now we are on the verge of accepting the path.
	 *
	 * There is one optimization we could do in the future: copy the path
	 * directly into the PLB using some kind of a callback.
	 */
	char *path = malloc(len);
	
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	int rc;
	if ((rc = ipc_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	
	rwlock_write_lock(&namespace_rwlock);
	lflag &= L_DIRECTORY;	/* sanitize lflag */
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(path, len, lflag | L_DESTROY, &lr, NULL);
	free(path);
	if (rc != EOK) {
		rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		return;
	}

	/*
	 * The name has already been unlinked by vfs_lookup_internal().
	 * We have to get and put the VFS node to ensure that it is
	 * VFS_DESTROY'ed after the last reference to it is dropped.
	 */
	vfs_node_t *node = vfs_node_get(&lr);
	node->lnkcnt--;
	rwlock_write_unlock(&namespace_rwlock);
	vfs_node_put(node);
	ipc_answer_0(rid, EOK);
}

/**
 * @}
 */ 
