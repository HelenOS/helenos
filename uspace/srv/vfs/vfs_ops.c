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

/** @addtogroup vfs
 * @{
 */

/**
 * @file vfs_ops.c
 * @brief Operations that VFS offers to its clients.
 */

#include "vfs.h"
#include <macros.h>
#include <stdint.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <ctype.h>
#include <assert.h>
#include <vfs/canonify.h>

/* Forward declarations of static functions. */
static errno_t vfs_truncate_internal(fs_handle_t, service_id_t, fs_index_t,
    aoff64_t);

/**
 * This rwlock prevents the race between a triplet-to-VFS-node resolution and a
 * concurrent VFS operation which modifies the file system namespace.
 */
FIBRIL_RWLOCK_INITIALIZE(namespace_rwlock);

static size_t shared_path(char *a, char *b)
{
	size_t res = 0;

	while (a[res] == b[res] && a[res] != 0)
		res++;

	if (a[res] == b[res])
		return res;

	res--;
	while (a[res] != '/')
		res--;
	return res;
}

/* This call destroys the file if and only if there are no hard links left. */
static void out_destroy(vfs_triplet_t *file)
{
	async_exch_t *exch = vfs_exchange_grab(file->fs_handle);
	async_msg_2(exch, VFS_OUT_DESTROY, (sysarg_t) file->service_id,
	    (sysarg_t) file->index);
	vfs_exchange_release(exch);
}

errno_t vfs_op_clone(int oldfd, int newfd, bool desc, int *out_fd)
{
	errno_t rc;

	/* Lookup the file structure corresponding to fd. */
	vfs_file_t *oldfile = vfs_file_get(oldfd);
	if (oldfile == NULL)
		return EBADF;

	assert(oldfile->node != NULL);

	/* If the file descriptors are the same, do nothing. */
	if (oldfd == newfd) {
		vfs_file_put(oldfile);
		return EOK;
	}

	if (newfd != -1) {
		/* Assign the old file to newfd. */
		rc = vfs_fd_assign(oldfile, newfd);
		*out_fd = newfd;
	} else {
		vfs_file_t *newfile;
		rc = vfs_fd_alloc(&newfile, desc, out_fd);
		if (rc == EOK) {
			newfile->node = oldfile->node;
			newfile->permissions = oldfile->permissions;
			vfs_node_addref(newfile->node);

			vfs_file_put(newfile);
		}
	}
	vfs_file_put(oldfile);

	return rc;
}

errno_t vfs_op_put(int fd)
{
	return vfs_fd_free(fd);
}

static errno_t vfs_connect_internal(service_id_t service_id, unsigned flags,
    unsigned instance, const char *options, const char *fsname,
    vfs_node_t **root)
{
	fs_handle_t fs_handle = 0;

	fibril_mutex_lock(&fs_list_lock);
	while (true) {
		fs_handle = fs_name_to_handle(instance, fsname, false);

		if (fs_handle != 0 || !(flags & VFS_MOUNT_BLOCKING))
			break;

		fibril_condvar_wait(&fs_list_cv, &fs_list_lock);
	}
	fibril_mutex_unlock(&fs_list_lock);

	if (fs_handle == 0)
		return ENOFS;

	/* Tell the mountee that it is being mounted. */
	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_grab(fs_handle);
	aid_t msg = async_send_1(exch, VFS_OUT_MOUNTED, (sysarg_t) service_id,
	    &answer);
	/* Send the mount options */
	errno_t rc = async_data_write_start(exch, options, str_size(options));
	if (rc != EOK) {
		async_forget(msg);
		vfs_exchange_release(exch);
		return rc;
	}

	async_wait_for(msg, &rc);
	if (rc != EOK) {
		vfs_exchange_release(exch);
		return rc;
	}

	vfs_lookup_res_t res;
	res.triplet.fs_handle = fs_handle;
	res.triplet.service_id = service_id;
	res.triplet.index = (fs_index_t) ipc_get_arg1(&answer);
	res.size = (int64_t) MERGE_LOUP32(ipc_get_arg2(&answer),
	    ipc_get_arg3(&answer));
	res.type = VFS_NODE_DIRECTORY;

	/* Add reference to the mounted root. */
	*root = vfs_node_get(&res);
	if (!*root) {
		aid_t msg = async_send_1(exch, VFS_OUT_UNMOUNTED,
		    (sysarg_t) service_id, NULL);
		async_forget(msg);
		vfs_exchange_release(exch);
		return ENOMEM;
	}

	vfs_exchange_release(exch);

	return EOK;
}

errno_t vfs_op_fsprobe(const char *fs_name, service_id_t sid,
    vfs_fs_probe_info_t *info)
{
	fs_handle_t fs_handle = 0;
	errno_t rc;
	errno_t retval;

	fibril_mutex_lock(&fs_list_lock);
	fs_handle = fs_name_to_handle(0, fs_name, false);
	fibril_mutex_unlock(&fs_list_lock);

	if (fs_handle == 0)
		return ENOFS;

	/* Send probe request to the file system server */
	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_grab(fs_handle);
	aid_t msg = async_send_1(exch, VFS_OUT_FSPROBE, (sysarg_t) sid,
	    &answer);
	if (msg == 0)
		return EINVAL;

	/* Read probe information */
	retval = async_data_read_start(exch, info, sizeof(*info));
	if (retval != EOK) {
		async_forget(msg);
		return retval;
	}

	async_wait_for(msg, &rc);
	vfs_exchange_release(exch);
	return rc;
}

errno_t vfs_op_mount(int mpfd, unsigned service_id, unsigned flags,
    unsigned instance, const char *opts, const char *fs_name, int *out_fd)
{
	errno_t rc;
	vfs_file_t *mp = NULL;
	vfs_file_t *file = NULL;
	*out_fd = -1;

	if (!(flags & VFS_MOUNT_CONNECT_ONLY)) {
		mp = vfs_file_get(mpfd);
		if (mp == NULL) {
			rc = EBADF;
			goto out;
		}

		if (mp->node->mount != NULL) {
			rc = EBUSY;
			goto out;
		}

		if (mp->node->type != VFS_NODE_DIRECTORY) {
			rc = ENOTDIR;
			goto out;
		}

		if (vfs_node_has_children(mp->node)) {
			rc = ENOTEMPTY;
			goto out;
		}
	}

	if (!(flags & VFS_MOUNT_NO_REF)) {
		rc = vfs_fd_alloc(&file, false, out_fd);
		if (rc != EOK) {
			goto out;
		}
	}

	vfs_node_t *root = NULL;

	fibril_rwlock_write_lock(&namespace_rwlock);

	rc = vfs_connect_internal(service_id, flags, instance, opts, fs_name,
	    &root);
	if (rc == EOK && !(flags & VFS_MOUNT_CONNECT_ONLY)) {
		vfs_node_addref(mp->node);
		vfs_node_addref(root);
		mp->node->mount = root;
	}

	fibril_rwlock_write_unlock(&namespace_rwlock);

	if (rc != EOK)
		goto out;

	if (flags & VFS_MOUNT_NO_REF) {
		vfs_node_delref(root);
	} else {
		assert(file != NULL);

		file->node = root;
		file->permissions = MODE_READ | MODE_WRITE | MODE_APPEND;
		file->open_read = false;
		file->open_write = false;
	}

out:
	if (mp)
		vfs_file_put(mp);
	if (file)
		vfs_file_put(file);

	if (rc != EOK && *out_fd >= 0) {
		vfs_fd_free(*out_fd);
		*out_fd = -1;
	}

	return rc;
}

errno_t vfs_op_open(int fd, int mode)
{
	if (mode == 0)
		return EINVAL;

	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	if ((mode & ~file->permissions) != 0) {
		vfs_file_put(file);
		return EPERM;
	}

	if (file->open_read || file->open_write) {
		vfs_file_put(file);
		return EBUSY;
	}

	file->open_read = (mode & MODE_READ) != 0;
	file->open_write = (mode & (MODE_WRITE | MODE_APPEND)) != 0;
	file->append = (mode & MODE_APPEND) != 0;

	if (!file->open_read && !file->open_write) {
		vfs_file_put(file);
		return EINVAL;
	}

	if (file->node->type == VFS_NODE_DIRECTORY && file->open_write) {
		file->open_read = file->open_write = false;
		vfs_file_put(file);
		return EINVAL;
	}

	errno_t rc = vfs_open_node_remote(file->node);
	if (rc != EOK) {
		file->open_read = file->open_write = false;
		vfs_file_put(file);
		return rc;
	}

	vfs_file_put(file);
	return EOK;
}

typedef errno_t (*rdwr_ipc_cb_t)(async_exch_t *, vfs_file_t *, aoff64_t,
    ipc_call_t *, bool, void *);

static errno_t rdwr_ipc_client(async_exch_t *exch, vfs_file_t *file, aoff64_t pos,
    ipc_call_t *answer, bool read, void *data)
{
	size_t *bytes = (size_t *) data;
	errno_t rc;

	/*
	 * Make a VFS_READ/VFS_WRITE request at the destination FS server
	 * and forward the IPC_M_DATA_READ/IPC_M_DATA_WRITE request to the
	 * destination FS server. The call will be routed as if sent by
	 * ourselves. Note that call arguments are immutable in this case so we
	 * don't have to bother.
	 */

	if (read) {
		rc = async_data_read_forward_4_1(exch, VFS_OUT_READ,
		    file->node->service_id, file->node->index,
		    LOWER32(pos), UPPER32(pos), answer);
	} else {
		rc = async_data_write_forward_4_1(exch, VFS_OUT_WRITE,
		    file->node->service_id, file->node->index,
		    LOWER32(pos), UPPER32(pos), answer);
	}

	*bytes = ipc_get_arg1(answer);
	return rc;
}

static errno_t rdwr_ipc_internal(async_exch_t *exch, vfs_file_t *file, aoff64_t pos,
    ipc_call_t *answer, bool read, void *data)
{
	rdwr_io_chunk_t *chunk = (rdwr_io_chunk_t *) data;

	if (exch == NULL)
		return ENOENT;

	aid_t msg = async_send_4(exch, read ? VFS_OUT_READ : VFS_OUT_WRITE,
	    file->node->service_id, file->node->index, LOWER32(pos),
	    UPPER32(pos), answer);
	if (msg == 0)
		return EINVAL;

	errno_t retval = async_data_read_start(exch, chunk->buffer, chunk->size);
	if (retval != EOK) {
		async_forget(msg);
		return retval;
	}

	errno_t rc;
	async_wait_for(msg, &rc);

	chunk->size = ipc_get_arg1(answer);

	return (errno_t) rc;
}

static errno_t vfs_rdwr(int fd, aoff64_t pos, bool read, rdwr_ipc_cb_t ipc_cb,
    void *ipc_cb_data)
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

	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	if ((read && !file->open_read) || (!read && !file->open_write)) {
		vfs_file_put(file);
		return EINVAL;
	}

	vfs_info_t *fs_info = fs_handle_to_info(file->node->fs_handle);
	assert(fs_info);

	bool rlock = read ||
	    (fs_info->concurrent_read_write && fs_info->write_retains_size);

	/*
	 * Lock the file's node so that no other client can read/write to it at
	 * the same time unless the FS supports concurrent reads/writes and its
	 * write implementation does not modify the file size.
	 */
	if (rlock)
		fibril_rwlock_read_lock(&file->node->contents_rwlock);
	else
		fibril_rwlock_write_lock(&file->node->contents_rwlock);

	if (file->node->type == VFS_NODE_DIRECTORY) {
		/*
		 * Make sure that no one is modifying the namespace
		 * while we are in readdir().
		 */

		if (!read) {
			if (rlock) {
				fibril_rwlock_read_unlock(
				    &file->node->contents_rwlock);
			} else {
				fibril_rwlock_write_unlock(
				    &file->node->contents_rwlock);
			}
			vfs_file_put(file);
			return EINVAL;
		}

		fibril_rwlock_read_lock(&namespace_rwlock);
	}

	async_exch_t *fs_exch = vfs_exchange_grab(file->node->fs_handle);

	if (!read && file->append)
		pos = file->node->size;

	/*
	 * Handle communication with the endpoint FS.
	 */
	ipc_call_t answer;
	errno_t rc = ipc_cb(fs_exch, file, pos, &answer, read, ipc_cb_data);

	vfs_exchange_release(fs_exch);

	if (file->node->type == VFS_NODE_DIRECTORY)
		fibril_rwlock_read_unlock(&namespace_rwlock);

	/* Unlock the VFS node. */
	if (rlock) {
		fibril_rwlock_read_unlock(&file->node->contents_rwlock);
	} else {
		/* Update the cached version of node's size. */
		if (rc == EOK) {
			file->node->size = MERGE_LOUP32(ipc_get_arg2(&answer),
			    ipc_get_arg3(&answer));
		}
		fibril_rwlock_write_unlock(&file->node->contents_rwlock);
	}

	vfs_file_put(file);

	return rc;
}

errno_t vfs_rdwr_internal(int fd, aoff64_t pos, bool read, rdwr_io_chunk_t *chunk)
{
	return vfs_rdwr(fd, pos, read, rdwr_ipc_internal, chunk);
}

errno_t vfs_op_read(int fd, aoff64_t pos, size_t *out_bytes)
{
	return vfs_rdwr(fd, pos, true, rdwr_ipc_client, out_bytes);
}

errno_t vfs_op_rename(int basefd, char *old, char *new)
{
	vfs_file_t *base_file = vfs_file_get(basefd);
	if (!base_file)
		return EBADF;

	vfs_node_t *base = base_file->node;
	vfs_node_addref(base);
	vfs_file_put(base_file);

	vfs_lookup_res_t base_lr;
	vfs_lookup_res_t old_lr;
	vfs_lookup_res_t new_lr_orig;
	bool orig_unlinked = false;

	errno_t rc;

	size_t shared = shared_path(old, new);

	/* Do not allow one path to be a prefix of the other. */
	if (old[shared] == 0 || new[shared] == 0) {
		vfs_node_put(base);
		return EINVAL;
	}
	assert(old[shared] == '/');
	assert(new[shared] == '/');

	fibril_rwlock_write_lock(&namespace_rwlock);

	/* Resolve the shared portion of the path first. */
	if (shared != 0) {
		old[shared] = 0;
		rc = vfs_lookup_internal(base, old, L_DIRECTORY, &base_lr);
		if (rc != EOK) {
			vfs_node_put(base);
			fibril_rwlock_write_unlock(&namespace_rwlock);
			return rc;
		}

		vfs_node_put(base);
		base = vfs_node_get(&base_lr);
		if (!base) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			return ENOMEM;
		}
		old[shared] = '/';
		old += shared;
		new += shared;
	}

	rc = vfs_lookup_internal(base, old, L_DISABLE_MOUNTS, &old_lr);
	if (rc != EOK) {
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}

	rc = vfs_lookup_internal(base, new, L_UNLINK | L_DISABLE_MOUNTS,
	    &new_lr_orig);
	if (rc == EOK) {
		orig_unlinked = true;
	} else if (rc != ENOENT) {
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}

	rc = vfs_link_internal(base, new, &old_lr.triplet);
	if (rc != EOK) {
		vfs_link_internal(base, old, &old_lr.triplet);
		if (orig_unlinked)
			vfs_link_internal(base, new, &new_lr_orig.triplet);
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}

	rc = vfs_lookup_internal(base, old, L_UNLINK | L_DISABLE_MOUNTS,
	    &old_lr);
	if (rc != EOK) {
		if (orig_unlinked)
			vfs_link_internal(base, new, &new_lr_orig.triplet);
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}

	/* If the node is not held by anyone, try to destroy it. */
	if (orig_unlinked) {
		vfs_node_t *node = vfs_node_peek(&new_lr_orig);
		if (!node)
			out_destroy(&new_lr_orig.triplet);
		else
			vfs_node_put(node);
	}

	vfs_node_put(base);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	return EOK;
}

errno_t vfs_op_resize(int fd, int64_t size)
{
	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	if (!file->open_write || file->node->type != VFS_NODE_FILE) {
		vfs_file_put(file);
		return EINVAL;
	}

	fibril_rwlock_write_lock(&file->node->contents_rwlock);

	errno_t rc = vfs_truncate_internal(file->node->fs_handle,
	    file->node->service_id, file->node->index, size);
	if (rc == EOK)
		file->node->size = size;

	fibril_rwlock_write_unlock(&file->node->contents_rwlock);
	vfs_file_put(file);
	return rc;
}

errno_t vfs_op_stat(int fd)
{
	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	vfs_node_t *node = file->node;

	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
	errno_t rc = async_data_read_forward_3_0(exch, VFS_OUT_STAT,
	    node->service_id, node->index, true);
	vfs_exchange_release(exch);

	vfs_file_put(file);
	return rc;
}

errno_t vfs_op_statfs(int fd)
{
	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	vfs_node_t *node = file->node;

	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
	errno_t rc = async_data_read_forward_3_0(exch, VFS_OUT_STATFS,
	    node->service_id, node->index, false);
	vfs_exchange_release(exch);

	vfs_file_put(file);
	return rc;
}

errno_t vfs_op_sync(int fd)
{
	vfs_file_t *file = vfs_file_get(fd);
	if (!file)
		return EBADF;

	async_exch_t *fs_exch = vfs_exchange_grab(file->node->fs_handle);

	aid_t msg;
	ipc_call_t answer;
	msg = async_send_2(fs_exch, VFS_OUT_SYNC, file->node->service_id,
	    file->node->index, &answer);

	vfs_exchange_release(fs_exch);

	errno_t rc;
	async_wait_for(msg, &rc);

	vfs_file_put(file);
	return rc;

}

static errno_t vfs_truncate_internal(fs_handle_t fs_handle, service_id_t service_id,
    fs_index_t index, aoff64_t size)
{
	async_exch_t *exch = vfs_exchange_grab(fs_handle);
	errno_t rc = async_req_4_0(exch, VFS_OUT_TRUNCATE,
	    (sysarg_t) service_id, (sysarg_t) index, LOWER32(size),
	    UPPER32(size));
	vfs_exchange_release(exch);

	return (errno_t) rc;
}

errno_t vfs_op_unlink(int parentfd, int expectfd, char *path)
{
	errno_t rc = EOK;
	vfs_file_t *parent = NULL;
	vfs_file_t *expect = NULL;

	if (parentfd == expectfd)
		return EINVAL;

	fibril_rwlock_write_lock(&namespace_rwlock);

	/*
	 * Files are retrieved in order of file descriptors, to prevent
	 * deadlock.
	 */
	if (parentfd < expectfd) {
		parent = vfs_file_get(parentfd);
		if (!parent) {
			rc = EBADF;
			goto exit;
		}
	}

	if (expectfd >= 0) {
		expect = vfs_file_get(expectfd);
		if (!expect) {
			rc = EBADF;
			goto exit;
		}
	}

	if (parentfd > expectfd) {
		parent = vfs_file_get(parentfd);
		if (!parent) {
			rc = EBADF;
			goto exit;
		}
	}

	assert(parent != NULL);

	if (expectfd >= 0) {
		vfs_lookup_res_t lr;
		rc = vfs_lookup_internal(parent->node, path, 0, &lr);
		if (rc != EOK)
			goto exit;

		vfs_node_t *found_node = vfs_node_peek(&lr);
		vfs_node_put(found_node);
		if (expect->node != found_node) {
			rc = ENOENT;
			goto exit;
		}

		vfs_file_put(expect);
		expect = NULL;
	}

	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(parent->node, path, L_UNLINK, &lr);
	if (rc != EOK)
		goto exit;

	/* If the node is not held by anyone, try to destroy it. */
	vfs_node_t *node = vfs_node_peek(&lr);
	if (!node)
		out_destroy(&lr.triplet);
	else
		vfs_node_put(node);

exit:
	if (path)
		free(path);
	if (parent)
		vfs_file_put(parent);
	if (expect)
		vfs_file_put(expect);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	return rc;
}

errno_t vfs_op_unmount(int mpfd)
{
	vfs_file_t *mp = vfs_file_get(mpfd);
	if (mp == NULL)
		return EBADF;

	if (mp->node->mount == NULL) {
		vfs_file_put(mp);
		return ENOENT;
	}

	fibril_rwlock_write_lock(&namespace_rwlock);

	/*
	 * Count the total number of references for the mounted file system. We
	 * are expecting at least one, which is held by the mount point.
	 * If we find more, it means that
	 * the file system cannot be gracefully unmounted at the moment because
	 * someone is working with it.
	 */
	if (vfs_nodes_refcount_sum_get(mp->node->mount->fs_handle,
	    mp->node->mount->service_id) != 1) {
		vfs_file_put(mp);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return EBUSY;
	}

	async_exch_t *exch = vfs_exchange_grab(mp->node->mount->fs_handle);
	errno_t rc = async_req_1_0(exch, VFS_OUT_UNMOUNTED,
	    mp->node->mount->service_id);
	vfs_exchange_release(exch);

	if (rc != EOK) {
		vfs_file_put(mp);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}

	vfs_node_forget(mp->node->mount);
	vfs_node_put(mp->node);
	mp->node->mount = NULL;

	fibril_rwlock_write_unlock(&namespace_rwlock);

	vfs_file_put(mp);
	return EOK;
}

errno_t vfs_op_wait_handle(bool high_fd, int *out_fd)
{
	return vfs_wait_handle_internal(high_fd, out_fd);
}

static inline bool walk_flags_valid(int flags)
{
	if ((flags & ~WALK_ALL_FLAGS) != 0)
		return false;
	if ((flags & WALK_MAY_CREATE) && (flags & WALK_MUST_CREATE))
		return false;
	if ((flags & WALK_REGULAR) && (flags & WALK_DIRECTORY))
		return false;
	if ((flags & WALK_MAY_CREATE) || (flags & WALK_MUST_CREATE)) {
		if (!(flags & WALK_DIRECTORY) && !(flags & WALK_REGULAR))
			return false;
	}
	return true;
}

static inline int walk_lookup_flags(int flags)
{
	int lflags = 0;
	if ((flags & WALK_MAY_CREATE) || (flags & WALK_MUST_CREATE))
		lflags |= L_CREATE;
	if (flags & WALK_MUST_CREATE)
		lflags |= L_EXCLUSIVE;
	if (flags & WALK_REGULAR)
		lflags |= L_FILE;
	if (flags & WALK_DIRECTORY)
		lflags |= L_DIRECTORY;
	if (flags & WALK_MOUNT_POINT)
		lflags |= L_MP;
	return lflags;
}

errno_t vfs_op_walk(int parentfd, int flags, char *path, int *out_fd)
{
	if (!walk_flags_valid(flags))
		return EINVAL;

	vfs_file_t *parent = vfs_file_get(parentfd);
	if (!parent)
		return EBADF;

	fibril_rwlock_read_lock(&namespace_rwlock);

	vfs_lookup_res_t lr;
	errno_t rc = vfs_lookup_internal(parent->node, path,
	    walk_lookup_flags(flags), &lr);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		vfs_file_put(parent);
		return rc;
	}

	vfs_node_t *node = vfs_node_get(&lr);
	if (!node) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		vfs_file_put(parent);
		return ENOMEM;
	}

	vfs_file_t *file;
	rc = vfs_fd_alloc(&file, false, out_fd);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		vfs_node_put(node);
		vfs_file_put(parent);
		return rc;
	}
	assert(file != NULL);

	file->node = node;
	file->permissions = parent->permissions;
	file->open_read = false;
	file->open_write = false;

	vfs_file_put(file);
	vfs_file_put(parent);

	fibril_rwlock_read_unlock(&namespace_rwlock);

	return EOK;
}

errno_t vfs_op_write(int fd, aoff64_t pos, size_t *out_bytes)
{
	return vfs_rdwr(fd, pos, false, rdwr_ipc_client, out_bytes);
}

/**
 * @}
 */
