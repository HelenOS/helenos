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
 * @file vfs_ops.c
 * @brief Operations that VFS offers to its clients.
 */

#include "vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bool.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <vfs/canonify.h>

/* Forward declarations of static functions. */
static int vfs_truncate_internal(fs_handle_t, dev_handle_t, fs_index_t, size_t);

/**
 * This rwlock prevents the race between a triplet-to-VFS-node resolution and a
 * concurrent VFS operation which modifies the file system namespace.
 */
FIBRIL_RWLOCK_INITIALIZE(namespace_rwlock);

vfs_pair_t rootfs = {
	.fs_handle = 0,
	.dev_handle = 0
};

static void vfs_mount_internal(ipc_callid_t rid, dev_handle_t dev_handle,
    fs_handle_t fs_handle, char *mp, char *opts)
{
	vfs_lookup_res_t mp_res;
	vfs_lookup_res_t mr_res;
	vfs_node_t *mp_node = NULL;
	vfs_node_t *mr_node;
	fs_index_t rindex;
	size_t rsize;
	unsigned rlnkcnt;
	ipcarg_t rc;
	int phone;
	aid_t msg;
	ipc_call_t answer;
	
	/* Resolve the path to the mountpoint. */
	fibril_rwlock_write_lock(&namespace_rwlock);
	if (rootfs.fs_handle) {
		/* We already have the root FS. */
		if (str_cmp(mp, "/") == 0) {
			/* Trying to mount root FS over root FS */
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, EBUSY);
			return;
		}
		
		rc = vfs_lookup_internal(mp, L_DIRECTORY, &mp_res, NULL);
		if (rc != EOK) {
			/* The lookup failed for some reason. */
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, rc);
			return;
		}
		
		mp_node = vfs_node_get(&mp_res);
		if (!mp_node) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, ENOMEM);
			return;
		}
		
		/*
		 * Now we hold a reference to mp_node.
		 * It will be dropped upon the corresponding VFS_IN_UNMOUNT.
		 * This prevents the mount point from being deleted.
		 */
	} else {
		/* We still don't have the root file system mounted. */
		if (str_cmp(mp, "/") == 0) {
			/*
			 * For this simple, but important case,
			 * we are almost done.
			 */
			
			/* Tell the mountee that it is being mounted. */
			phone = vfs_grab_phone(fs_handle);
			msg = async_send_1(phone, VFS_OUT_MOUNTED,
			    (ipcarg_t) dev_handle, &answer);
			/* send the mount options */
			rc = async_data_write_start(phone, (void *)opts,
			    str_size(opts));
			if (rc != EOK) {
				async_wait_for(msg, NULL);
				vfs_release_phone(phone);
				fibril_rwlock_write_unlock(&namespace_rwlock);
				ipc_answer_0(rid, rc);
				return;
			}
			async_wait_for(msg, &rc);
			vfs_release_phone(phone);
			
			if (rc != EOK) {
				fibril_rwlock_write_unlock(&namespace_rwlock);
				ipc_answer_0(rid, rc);
				return;
			}

			rindex = (fs_index_t) IPC_GET_ARG1(answer);
			rsize = (size_t) IPC_GET_ARG2(answer);
			rlnkcnt = (unsigned) IPC_GET_ARG3(answer);
			
			mr_res.triplet.fs_handle = fs_handle;
			mr_res.triplet.dev_handle = dev_handle;
			mr_res.triplet.index = rindex;
			mr_res.size = rsize;
			mr_res.lnkcnt = rlnkcnt;
			mr_res.type = VFS_NODE_DIRECTORY;
			
			rootfs.fs_handle = fs_handle;
			rootfs.dev_handle = dev_handle;
			
			/* Add reference to the mounted root. */
			mr_node = vfs_node_get(&mr_res); 
			assert(mr_node);
			
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, rc);
			return;
		} else {
			/*
			 * We can't resolve this without the root filesystem
			 * being mounted first.
			 */
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, ENOENT);
			return;
		}
	}
	
	/*
	 * At this point, we have all necessary pieces: file system and device
	 * handles, and we know the mount point VFS node.
	 */
	
	int mountee_phone = vfs_grab_phone(fs_handle);
	assert(mountee_phone >= 0);

	phone = vfs_grab_phone(mp_res.triplet.fs_handle);
	msg = async_send_4(phone, VFS_OUT_MOUNT,
	    (ipcarg_t) mp_res.triplet.dev_handle,
	    (ipcarg_t) mp_res.triplet.index,
	    (ipcarg_t) fs_handle,
	    (ipcarg_t) dev_handle, &answer);
	
	/* send connection */
	rc = async_req_1_0(phone, IPC_M_CONNECTION_CLONE, mountee_phone);
	if (rc != EOK) {
		async_wait_for(msg, NULL);
		vfs_release_phone(mountee_phone);
		vfs_release_phone(phone);
		/* Mount failed, drop reference to mp_node. */
		if (mp_node)
			vfs_node_put(mp_node);
		ipc_answer_0(rid, rc);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return;
	}

	vfs_release_phone(mountee_phone);
	
	/* send the mount options */
	rc = async_data_write_start(phone, (void *)opts, str_size(opts));
	if (rc != EOK) {
		async_wait_for(msg, NULL);
		vfs_release_phone(phone);
		/* Mount failed, drop reference to mp_node. */
		if (mp_node)
			vfs_node_put(mp_node);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		return;
	}
	async_wait_for(msg, &rc);
	vfs_release_phone(phone);
	
	if (rc == EOK) {
		rindex = (fs_index_t) IPC_GET_ARG1(answer);
		rsize = (size_t) IPC_GET_ARG2(answer);
		rlnkcnt = (unsigned) IPC_GET_ARG3(answer);
	
		mr_res.triplet.fs_handle = fs_handle;
		mr_res.triplet.dev_handle = dev_handle;
		mr_res.triplet.index = rindex;
		mr_res.size = rsize;
		mr_res.lnkcnt = rlnkcnt;
		mr_res.type = VFS_NODE_DIRECTORY;
	
		/* Add reference to the mounted root. */
		mr_node = vfs_node_get(&mr_res); 
		assert(mr_node);
	} else {
		/* Mount failed, drop reference to mp_node. */
		if (mp_node)
			vfs_node_put(mp_node);
	}

	ipc_answer_0(rid, rc);
	fibril_rwlock_write_unlock(&namespace_rwlock);
}

void vfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * We expect the library to do the device-name to device-handle
	 * translation for us, thus the device handle will arrive as ARG1
	 * in the request.
	 */
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	
	/*
	 * Mount flags are passed as ARG2.
	 */
	unsigned int flags = (unsigned int) IPC_GET_ARG2(*request);
	
	/*
	 * For now, don't make use of ARG3, but it can be used to
	 * carry mount options in the future.
	 */
	
	/* We want the client to send us the mount point. */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	/* Check whether size is reasonable wrt. the mount point. */
	if ((size < 1) || (size > MAX_PATH_LEN)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	/* Allocate buffer for the mount point data being received. */
	char *mp = malloc(size + 1);
	if (!mp) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	/* Deliver the mount point. */
	ipcarg_t retval = async_data_write_finalize(callid, mp, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(mp);
		return;
	}
	mp[size] = '\0';
	
	/* Now we expect to receive the mount options. */
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		free(mp);
		return;
	}

	/* Check the offered options size. */
	if (size > MAX_MNTOPTS_LEN) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		free(mp);
		return;
	}

	/* Allocate buffer for the mount options. */
	char *opts = (char *) malloc(size + 1);
	if (!opts) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		free(mp);
		return;
	}

	/* Deliver the mount options. */
	retval = async_data_write_finalize(callid, opts, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(mp);
		free(opts);
		return;
	}
	opts[size] = '\0';
	
	/*
	 * Now, we expect the client to send us data with the name of the file
	 * system.
	 */
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		free(mp);
		free(opts);
		return;
	}
	
	/*
	 * Don't receive more than is necessary for storing a full file system
	 * name.
	 */
	if ((size < 1) || (size > FS_NAME_MAXLEN)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		free(mp);
		free(opts);
		return;
	}
	
	/*
	 * Allocate buffer for file system name.
	 */
	char *fs_name = (char *) malloc(size + 1);
	if (fs_name == NULL) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		free(mp);
		free(opts);
		return;
	}
	
	/* Deliver the file system name. */
	retval = async_data_write_finalize(callid, fs_name, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(mp);
		free(opts);
		free(fs_name);
		return;
	}
	fs_name[size] = '\0';

	/*
	 * Wait for IPC_M_PING so that we can return an error if we don't know
	 * fs_name.
	 */
	ipc_call_t data;
	callid = async_get_call(&data);
	if (IPC_GET_METHOD(data) != IPC_M_PING) {
		ipc_answer_0(callid, ENOTSUP);
		ipc_answer_0(rid, ENOTSUP);
		free(mp);
		free(opts);
		free(fs_name);
		return;
	}

	/*
	 * Check if we know a file system with the same name as is in fs_name.
	 * This will also give us its file system handle.
	 */
	fibril_mutex_lock(&fs_head_lock);
	fs_handle_t fs_handle;
recheck:
	fs_handle = fs_name_to_handle(fs_name, false);
	if (!fs_handle) {
		if (flags & IPC_FLAG_BLOCKING) {
			fibril_condvar_wait(&fs_head_cv, &fs_head_lock);
			goto recheck;
		}
		
		fibril_mutex_unlock(&fs_head_lock);
		ipc_answer_0(callid, ENOENT);
		ipc_answer_0(rid, ENOENT);
		free(mp);
		free(fs_name);
		free(opts);
		return;
	}
	fibril_mutex_unlock(&fs_head_lock);
	
	/* Acknowledge that we know fs_name. */
	ipc_answer_0(callid, EOK);
	
	/* Do the mount */
	vfs_mount_internal(rid, dev_handle, fs_handle, mp, opts);
	free(mp);
	free(fs_name);
	free(opts);
}

void vfs_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	int rc;
	char *mp;
	vfs_lookup_res_t mp_res;
	vfs_lookup_res_t mr_res;
	vfs_node_t *mp_node;
	vfs_node_t *mr_node;
	int phone;

	/*
	 * Receive the mount point path.
	 */
	rc = async_data_string_receive(&mp, MAX_PATH_LEN);
	if (rc != EOK)
		ipc_answer_0(rid, rc);

	/*
	 * Taking the namespace lock will do two things for us. First, it will
	 * prevent races with other lookup operations. Second, it will stop new
	 * references to already existing VFS nodes and creation of new VFS
	 * nodes. This is because new references are added as a result of some
	 * lookup operation or at least of some operation which is protected by
	 * the namespace lock.
	 */
	fibril_rwlock_write_lock(&namespace_rwlock);
	
	/*
	 * Lookup the mounted root and instantiate it.
	 */
	rc = vfs_lookup_internal(mp, L_ROOT, &mr_res, NULL);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		free(mp);
		ipc_answer_0(rid, rc);
		return;
	}
	mr_node = vfs_node_get(&mr_res);
	if (!mr_node) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		free(mp);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Count the total number of references for the mounted file system. We
	 * are expecting at least two. One which we got above and one which we
	 * got when the file system was mounted. If we find more, it means that
	 * the file system cannot be gracefully unmounted at the moment because
	 * someone is working with it.
	 */
	if (vfs_nodes_refcount_sum_get(mr_node->fs_handle,
	    mr_node->dev_handle) != 2) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		vfs_node_put(mr_node);
		free(mp);
		ipc_answer_0(rid, EBUSY);
		return;
	}

	if (str_cmp(mp, "/") == 0) {

		/*
		 * Unmounting the root file system.
		 *
		 * In this case, there is no mount point node and we send
		 * VFS_OUT_UNMOUNTED directly to the mounted file system.
		 */

		free(mp);
		phone = vfs_grab_phone(mr_node->fs_handle);
		rc = async_req_1_0(phone, VFS_OUT_UNMOUNTED,
		    mr_node->dev_handle);
		vfs_release_phone(phone);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			vfs_node_put(mr_node);
			ipc_answer_0(rid, rc);
			return;
		}
		rootfs.fs_handle = 0;
		rootfs.dev_handle = 0;
	} else {

		/*
		 * Unmounting a non-root file system.
		 *
		 * We have a regular mount point node representing the parent
		 * file system, so we delegate the operation to it.
		 */

		rc = vfs_lookup_internal(mp, L_MP, &mp_res, NULL);
		free(mp);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			vfs_node_put(mr_node);
			ipc_answer_0(rid, rc);
			return;
		}
		vfs_node_t *mp_node = vfs_node_get(&mp_res);
		if (!mp_node) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			vfs_node_put(mr_node);
			ipc_answer_0(rid, ENOMEM);
			return;
		}

		phone = vfs_grab_phone(mp_node->fs_handle);
		rc = async_req_2_0(phone, VFS_OUT_UNMOUNT, mp_node->fs_handle,
		    mp_node->dev_handle);
		vfs_release_phone(phone);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			vfs_node_put(mp_node);
			vfs_node_put(mr_node);
			ipc_answer_0(rid, rc);
			return;
		}

		/* Drop the reference we got above. */
		vfs_node_put(mp_node);
		/* Drop the reference from when the file system was mounted. */
		vfs_node_put(mp_node);
	}


	/*
	 * All went well, the mounted file system was successfully unmounted.
	 * The only thing left is to forget the unmounted root VFS node.
	 */
	vfs_node_forget(mr_node);

	fibril_rwlock_write_unlock(&namespace_rwlock);
	ipc_answer_0(rid, EOK);
}

void vfs_open(ipc_callid_t rid, ipc_call_t *request)
{
	if (!vfs_files_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	/*
	 * The POSIX interface is open(path, oflag, mode).
	 * We can receive oflags and mode along with the VFS_IN_OPEN call;
	 * the path will need to arrive in another call.
	 *
	 * We also receive one private, non-POSIX set of flags called lflag
	 * used to pass information to vfs_lookup_internal().
	 */
	int lflag = IPC_GET_ARG1(*request);
	int oflag = IPC_GET_ARG2(*request);
	int mode = IPC_GET_ARG3(*request);
	size_t len;

	/* Ignore mode for now. */
	(void) mode;
	
	/*
	 * Make sure that we are called with exactly one of L_FILE and
	 * L_DIRECTORY. Make sure that the user does not pass L_OPEN,
	 * L_ROOT or L_MP.
	 */
	if (((lflag & (L_FILE | L_DIRECTORY)) == 0) ||
	    ((lflag & (L_FILE | L_DIRECTORY)) == (L_FILE | L_DIRECTORY)) ||
	    (lflag & (L_OPEN | L_ROOT | L_MP))) {
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	if (oflag & O_CREAT)
		lflag |= L_CREATE;
	if (oflag & O_EXCL)
		lflag |= L_EXCLUSIVE;
	
	ipc_callid_t callid;
	if (!async_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	char *path = malloc(len + 1);
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	int rc;
	if ((rc = async_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	path[len] = '\0';
	
	/*
	 * Avoid the race condition in which the file can be deleted before we
	 * find/create-and-lock the VFS node corresponding to the looked-up
	 * triplet.
	 */
	if (lflag & L_CREATE)
		fibril_rwlock_write_lock(&namespace_rwlock);
	else
		fibril_rwlock_read_lock(&namespace_rwlock);
	
	/* The path is now populated and we can call vfs_lookup_internal(). */
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(path, lflag | L_OPEN, &lr, NULL);
	if (rc != EOK) {
		if (lflag & L_CREATE)
			fibril_rwlock_write_unlock(&namespace_rwlock);
		else
			fibril_rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	
	/* Path is no longer needed. */
	free(path);
	
	vfs_node_t *node = vfs_node_get(&lr);
	if (lflag & L_CREATE)
		fibril_rwlock_write_unlock(&namespace_rwlock);
	else
		fibril_rwlock_read_unlock(&namespace_rwlock);
	
	/* Truncate the file if requested and if necessary. */
	if (oflag & O_TRUNC) {
		fibril_rwlock_write_lock(&node->contents_rwlock);
		if (node->size) {
			rc = vfs_truncate_internal(node->fs_handle,
			    node->dev_handle, node->index, 0);
			if (rc) {
				fibril_rwlock_write_unlock(&node->contents_rwlock);
				vfs_node_put(node);
				ipc_answer_0(rid, rc);
				return;
			}
			node->size = 0;
		}
		fibril_rwlock_write_unlock(&node->contents_rwlock);
	}
	
	/*
	 * Get ourselves a file descriptor and the corresponding vfs_file_t
	 * structure.
	 */
	int fd = vfs_fd_alloc((oflag & O_DESC) != 0);
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
	 * respective VFS_IN_CLOSE.
	 */
	vfs_node_addref(node);
	vfs_node_put(node);
	
	/* Success! Return the new file descriptor to the client. */
	ipc_answer_1(rid, EOK, fd);
}

void vfs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	// FIXME: check for sanity of the supplied fs, dev and index
	
	if (!vfs_files_init()) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	/*
	 * The interface is open_node(fs, dev, index, oflag).
	 */
	vfs_lookup_res_t lr;
	
	lr.triplet.fs_handle = IPC_GET_ARG1(*request);
	lr.triplet.dev_handle = IPC_GET_ARG2(*request);
	lr.triplet.index = IPC_GET_ARG3(*request);
	int oflag = IPC_GET_ARG4(*request);
	
	fibril_rwlock_read_lock(&namespace_rwlock);
	
	int rc = vfs_open_node_internal(&lr);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		return;
	}
	
	vfs_node_t *node = vfs_node_get(&lr);
	fibril_rwlock_read_unlock(&namespace_rwlock);
	
	/* Truncate the file if requested and if necessary. */
	if (oflag & O_TRUNC) {
		fibril_rwlock_write_lock(&node->contents_rwlock);
		if (node->size) {
			rc = vfs_truncate_internal(node->fs_handle,
			    node->dev_handle, node->index, 0);
			if (rc) {
				fibril_rwlock_write_unlock(&node->contents_rwlock);
				vfs_node_put(node);
				ipc_answer_0(rid, rc);
				return;
			}
			node->size = 0;
		}
		fibril_rwlock_write_unlock(&node->contents_rwlock);
	}
	
	/*
	 * Get ourselves a file descriptor and the corresponding vfs_file_t
	 * structure.
	 */
	int fd = vfs_fd_alloc((oflag & O_DESC) != 0);
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
	 * respective VFS_IN_CLOSE.
	 */
	vfs_node_addref(node);
	vfs_node_put(node);
	
	/* Success! Return the new file descriptor to the client. */
	ipc_answer_1(rid, EOK, fd);
}

void vfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	fibril_mutex_lock(&file->lock);
	int fs_phone = vfs_grab_phone(file->node->fs_handle);
	
	/* Make a VFS_OUT_SYMC request at the destination FS server. */
	aid_t msg;
	ipc_call_t answer;
	msg = async_send_2(fs_phone, VFS_OUT_SYNC, file->node->dev_handle,
	    file->node->index, &answer);

	/* Wait for reply from the FS server. */
	ipcarg_t rc;
	async_wait_for(msg, &rc);
	
	vfs_release_phone(fs_phone);
	fibril_mutex_unlock(&file->lock);
	
	ipc_answer_0(rid, rc);
}

static int vfs_close_internal(vfs_file_t *file)
{
	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	fibril_mutex_lock(&file->lock);
	
	if (file->refcnt <= 1) {
		/* Only close the file on the destination FS server
		   if there are no more file descriptors (except the
		   present one) pointing to this file. */
		
		int fs_phone = vfs_grab_phone(file->node->fs_handle);
		
		/* Make a VFS_OUT_CLOSE request at the destination FS server. */
		aid_t msg;
		ipc_call_t answer;
		msg = async_send_2(fs_phone, VFS_OUT_CLOSE, file->node->dev_handle,
		    file->node->index, &answer);
		
		/* Wait for reply from the FS server. */
		ipcarg_t rc;
		async_wait_for(msg, &rc);
		
		vfs_release_phone(fs_phone);
		fibril_mutex_unlock(&file->lock);
		
		return IPC_GET_ARG1(answer);
	}
	
	fibril_mutex_unlock(&file->lock);
	return EOK;
}

void vfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	int ret = vfs_close_internal(file);
	if (ret != EOK)
		ipc_answer_0(rid, ret);
	
	ret = vfs_fd_free(fd);
	ipc_answer_0(rid, ret);
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
		res = async_data_read_receive(&callid, NULL);
	else 
		res = async_data_write_receive(&callid, NULL);
	if (!res) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	fibril_mutex_lock(&file->lock);

	/*
	 * Lock the file's node so that no other client can read/write to it at
	 * the same time.
	 */
	if (read)
		fibril_rwlock_read_lock(&file->node->contents_rwlock);
	else
		fibril_rwlock_write_lock(&file->node->contents_rwlock);

	if (file->node->type == VFS_NODE_DIRECTORY) {
		/*
		 * Make sure that no one is modifying the namespace
		 * while we are in readdir().
		 */
		assert(read);
		fibril_rwlock_read_lock(&namespace_rwlock);
	}
	
	int fs_phone = vfs_grab_phone(file->node->fs_handle);	
	
	/* Make a VFS_READ/VFS_WRITE request at the destination FS server. */
	aid_t msg;
	ipc_call_t answer;
	if (!read && file->append)
		file->pos = file->node->size;
	msg = async_send_3(fs_phone, read ? VFS_OUT_READ : VFS_OUT_WRITE,
	    file->node->dev_handle, file->node->index, file->pos, &answer);
	
	/*
	 * Forward the IPC_M_DATA_READ/IPC_M_DATA_WRITE request to the
	 * destination FS server. The call will be routed as if sent by
	 * ourselves. Note that call arguments are immutable in this case so we
	 * don't have to bother.
	 */
	ipc_forward_fast(callid, fs_phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);

	/* Wait for reply from the FS server. */
	ipcarg_t rc;
	async_wait_for(msg, &rc);
	
	vfs_release_phone(fs_phone);
	
	size_t bytes = IPC_GET_ARG1(answer);

	if (file->node->type == VFS_NODE_DIRECTORY)
		fibril_rwlock_read_unlock(&namespace_rwlock);
	
	/* Unlock the VFS node. */
	if (read)
		fibril_rwlock_read_unlock(&file->node->contents_rwlock);
	else {
		/* Update the cached version of node's size. */
		if (rc == EOK)
			file->node->size = IPC_GET_ARG2(answer); 
		fibril_rwlock_write_unlock(&file->node->contents_rwlock);
	}
	
	/* Update the position pointer and unlock the open file. */
	if (rc == EOK)
		file->pos += bytes;
	fibril_mutex_unlock(&file->lock);
	
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
	fibril_mutex_lock(&file->lock);
	if (whence == SEEK_SET) {
		file->pos = off;
		fibril_mutex_unlock(&file->lock);
		ipc_answer_1(rid, EOK, off);
		return;
	}
	if (whence == SEEK_CUR) {
		if (file->pos + off < file->pos) {
			fibril_mutex_unlock(&file->lock);
			ipc_answer_0(rid, EOVERFLOW);
			return;
		}
		file->pos += off;
		newpos = file->pos;
		fibril_mutex_unlock(&file->lock);
		ipc_answer_1(rid, EOK, newpos);
		return;
	}
	if (whence == SEEK_END) {
		fibril_rwlock_read_lock(&file->node->contents_rwlock);
		size_t size = file->node->size;
		fibril_rwlock_read_unlock(&file->node->contents_rwlock);
		if (size + off < size) {
			fibril_mutex_unlock(&file->lock);
			ipc_answer_0(rid, EOVERFLOW);
			return;
		}
		newpos = size + off;
		file->pos = newpos;
		fibril_mutex_unlock(&file->lock);
		ipc_answer_1(rid, EOK, newpos);
		return;
	}
	fibril_mutex_unlock(&file->lock);
	ipc_answer_0(rid, EINVAL);
}

int
vfs_truncate_internal(fs_handle_t fs_handle, dev_handle_t dev_handle,
    fs_index_t index, size_t size)
{
	ipcarg_t rc;
	int fs_phone;
	
	fs_phone = vfs_grab_phone(fs_handle);
	rc = async_req_3_0(fs_phone, VFS_OUT_TRUNCATE, (ipcarg_t)dev_handle,
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
	fibril_mutex_lock(&file->lock);

	fibril_rwlock_write_lock(&file->node->contents_rwlock);
	rc = vfs_truncate_internal(file->node->fs_handle,
	    file->node->dev_handle, file->node->index, size);
	if (rc == EOK)
		file->node->size = size;
	fibril_rwlock_write_unlock(&file->node->contents_rwlock);

	fibril_mutex_unlock(&file->lock);
	ipc_answer_0(rid, (ipcarg_t)rc);
}

void vfs_fstat(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	ipcarg_t rc;

	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	ipc_callid_t callid;
	if (!async_data_read_receive(&callid, NULL)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	fibril_mutex_lock(&file->lock);

	int fs_phone = vfs_grab_phone(file->node->fs_handle);
	
	aid_t msg;
	msg = async_send_3(fs_phone, VFS_OUT_STAT, file->node->dev_handle,
	    file->node->index, true, NULL);
	ipc_forward_fast(callid, fs_phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
	async_wait_for(msg, &rc);
	vfs_release_phone(fs_phone);

	fibril_mutex_unlock(&file->lock);
	ipc_answer_0(rid, rc);
}

void vfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	size_t len;
	ipc_callid_t callid;

	if (!async_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *path = malloc(len + 1);
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	int rc;
	if ((rc = async_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	path[len] = '\0';

	if (!async_data_read_receive(&callid, NULL)) {
		free(path);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	vfs_lookup_res_t lr;
	fibril_rwlock_read_lock(&namespace_rwlock);
	rc = vfs_lookup_internal(path, L_NONE, &lr, NULL);
	free(path);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(callid, rc);
		ipc_answer_0(rid, rc);
		return;
	}
	vfs_node_t *node = vfs_node_get(&lr);
	if (!node) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	fibril_rwlock_read_unlock(&namespace_rwlock);

	int fs_phone = vfs_grab_phone(node->fs_handle);
	aid_t msg;
	msg = async_send_3(fs_phone, VFS_OUT_STAT, node->dev_handle,
	    node->index, false, NULL);
	ipc_forward_fast(callid, fs_phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
	
	ipcarg_t rv;
	async_wait_for(msg, &rv);
	vfs_release_phone(fs_phone);

	ipc_answer_0(rid, rv);

	vfs_node_put(node);
}

void vfs_mkdir(ipc_callid_t rid, ipc_call_t *request)
{
	int mode = IPC_GET_ARG1(*request);

	size_t len;
	ipc_callid_t callid;

	if (!async_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *path = malloc(len + 1);
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	int rc;
	if ((rc = async_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	path[len] = '\0';

	/* Ignore mode for now. */
	(void) mode;
	
	fibril_rwlock_write_lock(&namespace_rwlock);
	int lflag = L_DIRECTORY | L_CREATE | L_EXCLUSIVE;
	rc = vfs_lookup_internal(path, lflag, NULL, NULL);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	free(path);
	ipc_answer_0(rid, rc);
}

void vfs_unlink(ipc_callid_t rid, ipc_call_t *request)
{
	int lflag = IPC_GET_ARG1(*request);

	size_t len;
	ipc_callid_t callid;

	if (!async_data_write_receive(&callid, &len)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *path = malloc(len + 1);
	if (!path) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	int rc;
	if ((rc = async_data_write_finalize(callid, path, len))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	path[len] = '\0';
	
	fibril_rwlock_write_lock(&namespace_rwlock);
	lflag &= L_DIRECTORY;	/* sanitize lflag */
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(path, lflag | L_UNLINK, &lr, NULL);
	free(path);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		return;
	}

	/*
	 * The name has already been unlinked by vfs_lookup_internal().
	 * We have to get and put the VFS node to ensure that it is
	 * VFS_OUT_DESTROY'ed after the last reference to it is dropped.
	 */
	vfs_node_t *node = vfs_node_get(&lr);
	fibril_mutex_lock(&nodes_mutex);
	node->lnkcnt--;
	fibril_mutex_unlock(&nodes_mutex);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	vfs_node_put(node);
	ipc_answer_0(rid, EOK);
}

void vfs_rename(ipc_callid_t rid, ipc_call_t *request)
{
	size_t olen, nlen;
	ipc_callid_t callid;
	int rc;

	/* Retrieve the old path. */
	if (!async_data_write_receive(&callid, &olen)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *old = malloc(olen + 1);
	if (!old) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	if ((rc = async_data_write_finalize(callid, old, olen))) {
		ipc_answer_0(rid, rc);
		free(old);
		return;
	}
	old[olen] = '\0';
	
	/* Retrieve the new path. */
	if (!async_data_write_receive(&callid, &nlen)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		free(old);
		return;
	}
	char *new = malloc(nlen + 1);
	if (!new) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		free(old);
		return;
	}
	if ((rc = async_data_write_finalize(callid, new, nlen))) {
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	new[nlen] = '\0';

	char *oldc = canonify(old, &olen);
	char *newc = canonify(new, &nlen);
	if (!oldc || !newc) {
		ipc_answer_0(rid, EINVAL);
		free(old);
		free(new);
		return;
	}
	oldc[olen] = '\0';
	newc[nlen] = '\0';
	if ((!str_lcmp(newc, oldc, str_length(oldc))) &&
	    ((newc[str_length(oldc)] == '/') ||
	    (str_length(oldc) == 1) ||
	    (str_length(oldc) == str_length(newc)))) {
	    	/*
		 * oldc is a prefix of newc and either
		 * - newc continues with a / where oldc ends, or
		 * - oldc was / itself, or
		 * - oldc and newc are equal.
		 */
		ipc_answer_0(rid, EINVAL);
		free(old);
		free(new);
		return;
	}
	
	vfs_lookup_res_t old_lr;
	vfs_lookup_res_t new_lr;
	vfs_lookup_res_t new_par_lr;
	fibril_rwlock_write_lock(&namespace_rwlock);
	/* Lookup the node belonging to the old file name. */
	rc = vfs_lookup_internal(oldc, L_NONE, &old_lr, NULL);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	vfs_node_t *old_node = vfs_node_get(&old_lr);
	if (!old_node) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, ENOMEM);
		free(old);
		free(new);
		return;
	}
	/* Determine the path to the parent of the node with the new name. */
	char *parentc = str_dup(newc);
	if (!parentc) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	char *lastsl = str_rchr(parentc + 1, '/');
	if (lastsl)
		*lastsl = '\0';
	else
		parentc[1] = '\0';
	/* Lookup parent of the new file name. */
	rc = vfs_lookup_internal(parentc, L_NONE, &new_par_lr, NULL);
	free(parentc);	/* not needed anymore */
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	/* Check whether linking to the same file system instance. */
	if ((old_node->fs_handle != new_par_lr.triplet.fs_handle) ||
	    (old_node->dev_handle != new_par_lr.triplet.dev_handle)) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, EXDEV);	/* different file systems */
		free(old);
		free(new);
		return;
	}
	/* Destroy the old link for the new name. */
	vfs_node_t *new_node = NULL;
	rc = vfs_lookup_internal(newc, L_UNLINK, &new_lr, NULL);
	switch (rc) {
	case ENOENT:
		/* simply not in our way */
		break;
	case EOK:
		new_node = vfs_node_get(&new_lr);
		if (!new_node) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			ipc_answer_0(rid, ENOMEM);
			free(old);
			free(new);
			return;
		}
		fibril_mutex_lock(&nodes_mutex);
		new_node->lnkcnt--;
		fibril_mutex_unlock(&nodes_mutex);
		break;
	default:
		fibril_rwlock_write_unlock(&namespace_rwlock);
		ipc_answer_0(rid, ENOTEMPTY);
		free(old);
		free(new);
		return;
	}
	/* Create the new link for the new name. */
	rc = vfs_lookup_internal(newc, L_LINK, NULL, NULL, old_node->index);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		if (new_node)
			vfs_node_put(new_node);
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	fibril_mutex_lock(&nodes_mutex);
	old_node->lnkcnt++;
	fibril_mutex_unlock(&nodes_mutex);
	/* Destroy the link for the old name. */
	rc = vfs_lookup_internal(oldc, L_UNLINK, NULL, NULL);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		vfs_node_put(old_node);
		if (new_node)
			vfs_node_put(new_node);
		ipc_answer_0(rid, rc);
		free(old);
		free(new);
		return;
	}
	fibril_mutex_lock(&nodes_mutex);
	old_node->lnkcnt--;
	fibril_mutex_unlock(&nodes_mutex);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	vfs_node_put(old_node);
	if (new_node)
		vfs_node_put(new_node);
	free(old);
	free(new);
	ipc_answer_0(rid, EOK);
}

void vfs_dup(ipc_callid_t rid, ipc_call_t *request)
{
	int oldfd = IPC_GET_ARG1(*request);
	int newfd = IPC_GET_ARG2(*request);
	
	/* Lookup the file structure corresponding to oldfd. */
	vfs_file_t *oldfile = vfs_file_get(oldfd);
	if (!oldfile) {
		ipc_answer_0(rid, EBADF);
		return;
	}
	
	/* If the file descriptors are the same, do nothing. */
	if (oldfd == newfd) {
		ipc_answer_1(rid, EOK, newfd);
		return;
	}
	
	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	fibril_mutex_lock(&oldfile->lock);
	
	/* Lookup an open file structure possibly corresponding to newfd. */
	vfs_file_t *newfile = vfs_file_get(newfd);
	if (newfile) {
		/* Close the originally opened file. */
		int ret = vfs_close_internal(newfile);
		if (ret != EOK) {
			ipc_answer_0(rid, ret);
			return;
		}
		
		ret = vfs_fd_free(newfd);
		if (ret != EOK) {
			ipc_answer_0(rid, ret);
			return;
		}
	}
	
	/* Assign the old file to newfd. */
	int ret = vfs_fd_assign(oldfile, newfd);
	fibril_mutex_unlock(&oldfile->lock);
	
	if (ret != EOK)
		ipc_answer_0(rid, ret);
	else
		ipc_answer_1(rid, EOK, newfd);
}

/**
 * @}
 */
