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
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <vfs/canonify.h>
#include <vfs/vfs_mtab.h>

FIBRIL_MUTEX_INITIALIZE(mtab_list_lock);
LIST_INITIALIZE(mtab_list);
static size_t mtab_size = 0;

/* Forward declarations of static functions. */
static int vfs_truncate_internal(fs_handle_t, service_id_t, fs_index_t,
    aoff64_t);

/**
 * This rwlock prevents the race between a triplet-to-VFS-node resolution and a
 * concurrent VFS operation which modifies the file system namespace.
 */
FIBRIL_RWLOCK_INITIALIZE(namespace_rwlock);

vfs_node_t *root = NULL;

static int vfs_connect_internal(service_id_t service_id, unsigned flags, unsigned instance,
	char *options, char *fsname, vfs_node_t **root)
{
	fs_handle_t fs_handle = 0;
	
	fibril_mutex_lock(&fs_list_lock);
	while (1) {
		fs_handle = fs_name_to_handle(instance, fsname, false);
		
		if (fs_handle != 0 || !(flags & IPC_FLAG_BLOCKING)) {
			break;
		}
		
		fibril_condvar_wait(&fs_list_cv, &fs_list_lock);
	}
	fibril_mutex_unlock(&fs_list_lock);

	if (fs_handle == 0) {
		return ENOENT;
	}
	
	/* Tell the mountee that it is being mounted. */
	ipc_call_t answer;
	async_exch_t *exch = vfs_exchange_grab(fs_handle);
	aid_t msg = async_send_1(exch, VFS_OUT_MOUNTED, (sysarg_t) service_id, &answer);
	/* Send the mount options */
	sysarg_t rc = async_data_write_start(exch, options, str_size(options));
	if (rc != EOK) {
		async_forget(msg);
		vfs_exchange_release(exch);
		return rc;
	}
	async_wait_for(msg, &rc);
	vfs_exchange_release(exch);
	
	if (rc != EOK) {
		return rc;
	}
	
	vfs_lookup_res_t res;
	res.triplet.fs_handle = fs_handle;
	res.triplet.service_id = service_id;
	res.triplet.index = (fs_index_t) IPC_GET_ARG1(answer);
	res.size = (int64_t) MERGE_LOUP32(IPC_GET_ARG2(answer), IPC_GET_ARG3(answer));
	res.type = VFS_NODE_DIRECTORY;
	
	/* Add reference to the mounted root. */
	*root = vfs_node_get(&res); 
	assert(*root);
			
	return EOK;
}

static int vfs_mount_internal(service_id_t service_id, unsigned flags, unsigned instance,
	char *opts, char *fs_name, char *mp)
{
	/* Resolve the path to the mountpoint. */
	
	if (root == NULL) {
		/* We still don't have the root file system mounted. */
		if (str_cmp(mp, "/") != 0) {
			/*
			 * We can't resolve this without the root filesystem
			 * being mounted first.
			 */
			return ENOENT;
		}
		
		return vfs_connect_internal(service_id, flags, instance, opts, fs_name, &root);
	}
	
	/* We already have the root FS. */
	if (str_cmp(mp, "/") == 0) {
		/* Trying to mount root FS over root FS */
		return EBUSY;
	}
	
	vfs_lookup_res_t mp_res;
	int rc = vfs_lookup_internal(root, mp, L_DIRECTORY, &mp_res);
	if (rc != EOK) {
		/* The lookup failed. */
		return rc;
	}
	
	vfs_node_t *mp_node;
	mp_node = vfs_node_get(&mp_res);
	if (!mp_node) {
		return ENOMEM;
	}
	
	if (mp_node->mount != NULL) {
		return EBUSY;
	}
	
	if (mp_node->type != VFS_NODE_DIRECTORY) {
		printf("%s node not a directory, type=%d\n", mp, mp_node->type);
		return ENOTDIR;
	}
	
	if (vfs_node_has_children(mp_node)) {
		return ENOTEMPTY;
	}
	
	vfs_node_t *mountee;
	
	rc = vfs_connect_internal(service_id, flags, instance, opts, fs_name, &mountee);
	if (rc != EOK) {
		vfs_node_put(mp_node);
		return ENOMEM;
	}
	
	mp_node->mount = mountee;
	/* The two references to nodes are held by the mount so that they cannot be freed.
	 * They are removed in detach_internal().
	 */
	return EOK;
}

void vfs_mount_srv(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * We expect the library to do the device-name to device-handle
	 * translation for us, thus the device handle will arrive as ARG1
	 * in the request.
	 */
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*request);
	
	/*
	 * Mount flags are passed as ARG2.
	 */
	unsigned int flags = (unsigned int) IPC_GET_ARG2(*request);
	
	/*
	 * Instance number is passed as ARG3.
	 */
	unsigned int instance = IPC_GET_ARG3(*request);

	/* We want the client to send us the mount point. */
	char *mp;
	int rc = async_data_write_accept((void **) &mp, true, 0, MAX_PATH_LEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	
	/* Now we expect to receive the mount options. */
	char *opts;
	rc = async_data_write_accept((void **) &opts, true, 0, MAX_MNTOPTS_LEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		free(mp);
		return;
	}
	
	/*
	 * Now, we expect the client to send us data with the name of the file
	 * system.
	 */
	char *fs_name;
	rc = async_data_write_accept((void **) &fs_name, true, 0,
	    FS_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		free(mp);
		free(opts);
		return;
	}
	
	/* Add the filesystem info to the list of mounted filesystems */
	mtab_ent_t *mtab_ent = malloc(sizeof(mtab_ent_t));
	if (!mtab_ent) {
		async_answer_0(rid, ENOMEM);
		free(mp);
		free(fs_name);
		free(opts);
		return;
	}
	
	/* Mount the filesystem. */
	fibril_rwlock_write_lock(&namespace_rwlock);
	rc = vfs_mount_internal(service_id, flags, instance, opts, fs_name, mp);
	fibril_rwlock_write_unlock(&namespace_rwlock);

	/* Add the filesystem info to the list of mounted filesystems */
	if (rc == EOK) {
		str_cpy(mtab_ent->mp, MAX_PATH_LEN, mp);
		str_cpy(mtab_ent->fs_name, FS_NAME_MAXLEN, fs_name);
		str_cpy(mtab_ent->opts, MAX_MNTOPTS_LEN, opts);
		mtab_ent->instance = instance;
		mtab_ent->service_id = service_id;

		link_initialize(&mtab_ent->link);

		fibril_mutex_lock(&mtab_list_lock);
		list_append(&mtab_ent->link, &mtab_list);
		mtab_size++;
		fibril_mutex_unlock(&mtab_list_lock);
	}
	
	async_answer_0(rid, rc);

	free(mp);
	free(fs_name);
	free(opts);
}

void vfs_unmount_srv(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * Receive the mount point path.
	 */
	char *mp;
	int rc = async_data_write_accept((void **) &mp, true, 0, MAX_PATH_LEN,
	    0, NULL);
	if (rc != EOK)
		async_answer_0(rid, rc);
	
	/*
	 * Taking the namespace lock will do two things for us. First, it will
	 * prevent races with other lookup operations. Second, it will stop new
	 * references to already existing VFS nodes and creation of new VFS
	 * nodes. This is because new references are added as a result of some
	 * lookup operation or at least of some operation which is protected by
	 * the namespace lock.
	 */
	fibril_rwlock_write_lock(&namespace_rwlock);
		
	if (str_cmp(mp, "/") == 0) {
		free(mp);
		
		/*
		 * Unmounting the root file system.
		 *
		 * In this case, there is no mount point node and we send
		 * VFS_OUT_UNMOUNTED directly to the mounted file system.
		 */
		
		if (!root) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			async_answer_0(rid, ENOENT);
			return;
		}
		
		/*
		 * Count the total number of references for the mounted file system. We
		 * are expecting at least one, which we got when the file system was mounted.
		 * If we find more, it means that
		 * the file system cannot be gracefully unmounted at the moment because
		 * someone is working with it.
		 */
		if (vfs_nodes_refcount_sum_get(root->fs_handle, root->service_id) != 1) {
			fibril_rwlock_write_unlock(&namespace_rwlock);
			async_answer_0(rid, EBUSY);
			return;
		}
		
		async_exch_t *exch = vfs_exchange_grab(root->fs_handle);
		rc = async_req_1_0(exch, VFS_OUT_UNMOUNTED, root->service_id);
		vfs_exchange_release(exch);
		
		fibril_rwlock_write_unlock(&namespace_rwlock);
		if (rc == EOK) {
			vfs_node_forget(root);
			root = NULL;
		}
		async_answer_0(rid, rc);
		return;
	}
	
	/*
	 * Lookup the mounted root and instantiate it.
	 */
	vfs_lookup_res_t mp_res;
	rc = vfs_lookup_internal(root, mp, L_MP, &mp_res);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		free(mp);
		async_answer_0(rid, rc);
		return;
	}
	vfs_node_t *mp_node = vfs_node_get(&mp_res);
	if (!mp_node) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		free(mp);
		async_answer_0(rid, ENOMEM);
		return;
	}
	
	if (mp_node->mount == NULL) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		vfs_node_put(mp_node);
		free(mp);
		async_answer_0(rid, ENOENT);
		return;
	}
	
	/*
	 * Count the total number of references for the mounted file system. We
	 * are expecting at least one, which we got when the file system was mounted.
	 * If we find more, it means that
	 * the file system cannot be gracefully unmounted at the moment because
	 * someone is working with it.
	 */
	if (vfs_nodes_refcount_sum_get(mp_node->mount->fs_handle, mp_node->mount->service_id) != 1) {
		fibril_rwlock_write_unlock(&namespace_rwlock);
		vfs_node_put(mp_node);
		free(mp);
		async_answer_0(rid, EBUSY);
		return;
	}
	
	/* Unmount the filesystem. */
	async_exch_t *exch = vfs_exchange_grab(mp_node->mount->fs_handle);
	rc = async_req_1_0(exch, VFS_OUT_UNMOUNTED, mp_node->mount->service_id);
	vfs_exchange_release(exch);
	
	vfs_node_forget(mp_node->mount);
	mp_node->mount = NULL;
	
	vfs_node_put(mp_node);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	
	fibril_mutex_lock(&mtab_list_lock);
	int found = 0;

	list_foreach(mtab_list, link, mtab_ent_t, mtab_ent) {
		if (str_cmp(mtab_ent->mp, mp) == 0) {
			list_remove(&mtab_ent->link);
			mtab_size--;
			free(mtab_ent);
			found = 1;
			break;
		}
	}
	assert(found);
	fibril_mutex_unlock(&mtab_list_lock);

	free(mp);	
	
	async_answer_0(rid, EOK);
	return;
}

static inline bool walk_flags_valid(int flags)
{
	if ((flags&~WALK_ALL_FLAGS) != 0) {
		return false;
	}
	if ((flags&WALK_MAY_CREATE) && (flags&WALK_MUST_CREATE)) {
		return false;
	}
	if ((flags&WALK_REGULAR) && (flags&WALK_DIRECTORY)) {
		return false;
	}
	if ((flags&WALK_MAY_CREATE) || (flags&WALK_MUST_CREATE)) {
		if (!(flags&WALK_DIRECTORY) && !(flags&WALK_REGULAR)) {
			return false;
		}
	}
	return true;
}

static inline int walk_lookup_flags(int flags)
{
	int lflags = 0;
	if (flags&WALK_MAY_CREATE || flags&WALK_MUST_CREATE) {
		lflags |= L_CREATE;
	}
	if (flags&WALK_MUST_CREATE) {
		lflags |= L_EXCLUSIVE;
	}
	if (flags&WALK_REGULAR) {
		lflags |= L_FILE;
	}
	if (flags&WALK_DIRECTORY) {
		lflags |= L_DIRECTORY;
	}
	return lflags;
}

void vfs_walk(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * Parent is our relative root for file lookup.
	 * For defined flags, see <ipc/vfs.h>.
	 */
	int parentfd = IPC_GET_ARG1(*request);
	int flags = IPC_GET_ARG2(*request);
	
	if (!walk_flags_valid(flags)) {
		async_answer_0(rid, EINVAL);
		return;
	}
	
	char *path;
	int rc = async_data_write_accept((void **)&path, true, 0, 0, 0, NULL);
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *parent = NULL;
	vfs_node_t *parent_node = root;
	// TODO: Client-side root.
	if (parentfd != -1) {
		parent = vfs_file_get(parentfd);
		if (!parent) {
			free(path);
			async_answer_0(rid, EBADF);
			return;
		}
		parent_node = parent->node;
	}
	
	fibril_rwlock_read_lock(&namespace_rwlock);
	
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(parent_node, path, walk_lookup_flags(flags), &lr);
	free(path);

	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		if (parent) {
			vfs_file_put(parent);
		}
		async_answer_0(rid, rc);
		return;
	}
	
	vfs_node_t *node = vfs_node_get(&lr);
	
	vfs_file_t *file;
	int fd = vfs_fd_alloc(&file, false);
	if (fd < 0) {
		vfs_node_put(node);
		if (parent) {
			vfs_file_put(parent);
		}
		async_answer_0(rid, fd);
		return;
	}
	assert(file != NULL);
	
	file->node = node;
	if (parent) {
		file->permissions = parent->permissions;
	} else {
		file->permissions = MODE_READ | MODE_WRITE | MODE_APPEND;
	}
	file->open_read = false;
	file->open_write = false;
	
	vfs_file_put(file);
	if (parent) {
		vfs_file_put(parent);
	}
	
	fibril_rwlock_read_unlock(&namespace_rwlock);

	async_answer_1(rid, EOK, fd);
}

void vfs_open2(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int flags = IPC_GET_ARG2(*request);

	if (flags == 0) {
		async_answer_0(rid, EINVAL);
		return;
	}

	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		async_answer_0(rid, EBADF);
		return;
	}
	
	if ((flags & ~file->permissions) != 0) {
		vfs_file_put(file);
		async_answer_0(rid, EPERM);
		return;
	}
	
	file->open_read = (flags & MODE_READ) != 0;
	file->open_write = (flags & (MODE_WRITE | MODE_APPEND)) != 0;
	file->append = (flags & MODE_APPEND) != 0;
	
	if (!file->open_read && !file->open_write) {
		vfs_file_put(file);
		async_answer_0(rid, EINVAL);
		return;
	}
	
	if (file->node->type == VFS_NODE_DIRECTORY && file->open_write) {
		file->open_read = file->open_write = false;
		vfs_file_put(file);
		async_answer_0(rid, EINVAL);
		return;
	}
	
	int rc = vfs_open_node_remote(file->node);
	if (rc != EOK) {
		file->open_read = file->open_write = false;
		vfs_file_put(file);
		async_answer_0(rid, rc);
		return;
	}
	
	vfs_file_put(file);
	async_answer_0(rid, EOK);
}

void vfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		async_answer_0(rid, ENOENT);
		return;
	}
	
	/*
	 * Lock the open file structure so that no other thread can manipulate
	 * the same open file at a time.
	 */
	async_exch_t *fs_exch = vfs_exchange_grab(file->node->fs_handle);
	
	/* Make a VFS_OUT_SYMC request at the destination FS server. */
	aid_t msg;
	ipc_call_t answer;
	msg = async_send_2(fs_exch, VFS_OUT_SYNC, file->node->service_id,
	    file->node->index, &answer);
	
	vfs_exchange_release(fs_exch);
	
	/* Wait for reply from the FS server. */
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	vfs_file_put(file);
	async_answer_0(rid, rc);
}

void vfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int ret = vfs_fd_free(fd);
	async_answer_0(rid, ret);
}

typedef int (* rdwr_ipc_cb_t)(async_exch_t *, vfs_file_t *, ipc_call_t *,
    bool, void *);

static int rdwr_ipc_client(async_exch_t *exch, vfs_file_t *file,
    ipc_call_t *answer, bool read, void *data)
{
	size_t *bytes = (size_t *) data;
	int rc;

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
		    LOWER32(file->pos), UPPER32(file->pos), answer);
	} else {
		rc = async_data_write_forward_4_1(exch, VFS_OUT_WRITE,
		    file->node->service_id, file->node->index,
		    LOWER32(file->pos), UPPER32(file->pos), answer);
	}

	*bytes = IPC_GET_ARG1(*answer);
	return rc;
}

static int rdwr_ipc_internal(async_exch_t *exch, vfs_file_t *file,
    ipc_call_t *answer, bool read, void *data)
{
	rdwr_io_chunk_t *chunk = (rdwr_io_chunk_t *) data;

	if (exch == NULL)
		return ENOENT;
	
	aid_t msg = async_send_fast(exch, read ? VFS_OUT_READ : VFS_OUT_WRITE,
	    file->node->service_id, file->node->index, LOWER32(file->pos),
	    UPPER32(file->pos), answer);
	if (msg == 0)
		return EINVAL;

	int retval = async_data_read_start(exch, chunk->buffer, chunk->size);
	if (retval != EOK) {
		async_forget(msg);
		return retval;
	}
	
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	chunk->size = IPC_GET_ARG1(*answer); 

	return (int) rc;
}

static int vfs_rdwr(int fd, bool read, rdwr_ipc_cb_t ipc_cb, void *ipc_cb_data)
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
		return ENOENT;
	
	if ((read && !file->open_read) || (!read && !file->open_write)) {
		vfs_file_put(file);
		return EINVAL;
	}
	
	vfs_info_t *fs_info = fs_handle_to_info(file->node->fs_handle);
	assert(fs_info);
	
	/*
	 * Lock the file's node so that no other client can read/write to it at
	 * the same time unless the FS supports concurrent reads/writes and its
	 * write implementation does not modify the file size.
	 */
	if ((read) ||
	    ((fs_info->concurrent_read_write) && (fs_info->write_retains_size)))
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
	
	async_exch_t *fs_exch = vfs_exchange_grab(file->node->fs_handle);
	
	if (!read && file->append)
		file->pos = file->node->size;
	
	/*
	 * Handle communication with the endpoint FS.
	 */
	ipc_call_t answer;
	int rc = ipc_cb(fs_exch, file, &answer, read, ipc_cb_data);
	
	vfs_exchange_release(fs_exch);
	
	size_t bytes = IPC_GET_ARG1(answer);
	
	if (file->node->type == VFS_NODE_DIRECTORY) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
	}
	
	/* Unlock the VFS node. */
	if ((read) ||
	    ((fs_info->concurrent_read_write) && (fs_info->write_retains_size)))
		fibril_rwlock_read_unlock(&file->node->contents_rwlock);
	else {
		/* Update the cached version of node's size. */
		if (rc == EOK)
			file->node->size = MERGE_LOUP32(IPC_GET_ARG2(answer),
			    IPC_GET_ARG3(answer));
		fibril_rwlock_write_unlock(&file->node->contents_rwlock);
	}
	
	/* Update the position pointer and unlock the open file. */
	if (rc == EOK)
		file->pos += bytes;
	vfs_file_put(file);	

	return rc;
}
	
static void vfs_rdwr_client(ipc_callid_t rid, ipc_call_t *request, bool read)
{
	size_t bytes = 0;	
	int rc = vfs_rdwr(IPC_GET_ARG1(*request), read, rdwr_ipc_client,
	    &bytes);
	async_answer_1(rid, rc, bytes);
}

int vfs_rdwr_internal(int fd, bool read, rdwr_io_chunk_t *chunk)
{
	return vfs_rdwr(fd, read, rdwr_ipc_internal, chunk);
}

void vfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	vfs_rdwr_client(rid, request, true);
}

void vfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	vfs_rdwr_client(rid, request, false);
}

void vfs_seek(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = (int) IPC_GET_ARG1(*request);
	off64_t off = (off64_t) MERGE_LOUP32(IPC_GET_ARG2(*request),
	    IPC_GET_ARG3(*request));
	int whence = (int) IPC_GET_ARG4(*request);
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		async_answer_0(rid, ENOENT);
		return;
	}
	
	off64_t newoff;
	switch (whence) {
	case SEEK_SET:
		if (off >= 0) {
			file->pos = (aoff64_t) off;
			vfs_file_put(file);
			async_answer_1(rid, EOK, off);
			return;
		}
		break;
	case SEEK_CUR:
		if ((off >= 0) && (file->pos + off < file->pos)) {
			vfs_file_put(file);
			async_answer_0(rid, EOVERFLOW);
			return;
		}
		
		if ((off < 0) && (file->pos < (aoff64_t) -off)) {
			vfs_file_put(file);
			async_answer_0(rid, EOVERFLOW);
			return;
		}
		
		file->pos += off;
		newoff = (file->pos > OFF64_MAX) ? OFF64_MAX : file->pos;
		
		vfs_file_put(file);
		async_answer_2(rid, EOK, LOWER32(newoff),
		    UPPER32(newoff));
		return;
	case SEEK_END:
		fibril_rwlock_read_lock(&file->node->contents_rwlock);
		aoff64_t size = vfs_node_get_size(file->node);
		
		if ((off >= 0) && (size + off < size)) {
			fibril_rwlock_read_unlock(&file->node->contents_rwlock);
			vfs_file_put(file);
			async_answer_0(rid, EOVERFLOW);
			return;
		}
		
		if ((off < 0) && (size < (aoff64_t) -off)) {
			fibril_rwlock_read_unlock(&file->node->contents_rwlock);
			vfs_file_put(file);
			async_answer_0(rid, EOVERFLOW);
			return;
		}
		
		file->pos = size + off;
		newoff = (file->pos > OFF64_MAX) ?  OFF64_MAX : file->pos;
		
		fibril_rwlock_read_unlock(&file->node->contents_rwlock);
		vfs_file_put(file);
		async_answer_2(rid, EOK, LOWER32(newoff), UPPER32(newoff));
		return;
	}
	
	vfs_file_put(file);
	async_answer_0(rid, EINVAL);
}

int vfs_truncate_internal(fs_handle_t fs_handle, service_id_t service_id,
    fs_index_t index, aoff64_t size)
{
	async_exch_t *exch = vfs_exchange_grab(fs_handle);
	sysarg_t rc = async_req_4_0(exch, VFS_OUT_TRUNCATE,
	    (sysarg_t) service_id, (sysarg_t) index, LOWER32(size),
	    UPPER32(size));
	vfs_exchange_release(exch);
	
	return (int) rc;
}

void vfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	aoff64_t size = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG2(*request),
	    IPC_GET_ARG3(*request));
	int rc;

	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		async_answer_0(rid, ENOENT);
		return;
	}

	fibril_rwlock_write_lock(&file->node->contents_rwlock);
	rc = vfs_truncate_internal(file->node->fs_handle,
	    file->node->service_id, file->node->index, size);
	if (rc == EOK)
		file->node->size = size;
	fibril_rwlock_write_unlock(&file->node->contents_rwlock);

	vfs_file_put(file);
	async_answer_0(rid, (sysarg_t)rc);
}

void vfs_fstat(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	sysarg_t rc;

	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		async_answer_0(rid, ENOENT);
		return;
	}
	assert(file->node);

	ipc_callid_t callid;
	if (!async_data_read_receive(&callid, NULL)) {
		vfs_file_put(file);
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	async_exch_t *exch = vfs_exchange_grab(file->node->fs_handle);
	assert(exch);
	
	aid_t msg;
	msg = async_send_3(exch, VFS_OUT_STAT, file->node->service_id,
	    file->node->index, true, NULL);
	assert(msg);
	async_forward_fast(callid, exch, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
	
	vfs_exchange_release(exch);
	
	async_wait_for(msg, &rc);
	
	vfs_file_put(file);
	async_answer_0(rid, rc);
}

static void out_destroy(vfs_triplet_t *file)
{
	async_exch_t *exch = vfs_exchange_grab(file->fs_handle);
	async_msg_2(exch, VFS_OUT_DESTROY,
		(sysarg_t) file->service_id, (sysarg_t) file->index);
	vfs_exchange_release(exch);
}

void vfs_unlink2(ipc_callid_t rid, ipc_call_t *request)
{
	int rc;
	char *path;
	vfs_file_t *parent = NULL;
	vfs_file_t *expect = NULL;
	vfs_node_t *parent_node = root;
	
	int parentfd = IPC_GET_ARG1(*request);
	int expectfd = IPC_GET_ARG2(*request);
	int wflag = IPC_GET_ARG3(*request);
	
	rc = async_data_write_accept((void **) &path, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	
	fibril_rwlock_write_lock(&namespace_rwlock);
	
	int lflag = (wflag&WALK_DIRECTORY) ? L_DIRECTORY: 0;

	/* Files are retrieved in order of file descriptors, to prevent deadlock. */
	if (parentfd >= 0 && parentfd < expectfd) {
		parent = vfs_file_get(parentfd);
		if (!parent) {
			rc = ENOENT;
			goto exit;
		}
	}
	
	if (expectfd >= 0) {
		expect = vfs_file_get(expectfd);
		if (!expect) {
			rc = ENOENT;
			goto exit;
		}
	}
	
	if (parentfd >= 0 && parentfd >= expectfd) {
		parent = vfs_file_get(parentfd);
		if (!parent) {
			rc = ENOENT;
			goto exit;
		}
	}
	
	if (parent) {
		parent_node = parent->node;
	}
	
	if (expectfd >= 0) {
		vfs_lookup_res_t lr;
		rc = vfs_lookup_internal(parent_node, path, lflag, &lr);
		if (rc != EOK) {
			goto exit;
		}
		
		if (__builtin_memcmp(&lr.triplet, expect->node, sizeof(vfs_triplet_t)) != 0) {
			rc = ENOENT;
			goto exit;
		}
		
		vfs_file_put(expect);
		expect = NULL;
	}
	
	vfs_lookup_res_t lr;
	rc = vfs_lookup_internal(parent_node, path, lflag | L_UNLINK, &lr);
	if (rc != EOK) {
		goto exit;
	}

	/* If the node is not held by anyone, try to destroy it. */
	if (vfs_node_peek(&lr) == NULL) {
		out_destroy(&lr.triplet);
	}

exit:
	if (path) {
		free(path);
	}
	if (parent) {
		vfs_file_put(parent);
	}
	if (expect) {
		vfs_file_put(expect);
	}
	fibril_rwlock_write_unlock(&namespace_rwlock);
	async_answer_0(rid, rc);
}

static size_t shared_path(char *a, char *b)
{
	size_t res = 0;
	
	while (a[res] == b[res] && a[res] != 0) {
		res++;
	}
	
	if (a[res] == b[res]) {
		return res;
	}
	
	res--;
	while (a[res] != '/') {
		res--;
	}
	return res;
}

static int vfs_rename_internal(vfs_node_t *base, char *old, char *new)
{
	assert(base != NULL);
	assert(old != NULL);
	assert(new != NULL);
	
	vfs_lookup_res_t base_lr;
	vfs_lookup_res_t old_lr;
	vfs_lookup_res_t new_lr_orig;
	bool orig_unlinked = false;
	
	int rc;
	
	size_t shared = shared_path(old, new);
	
	/* Do not allow one path to be a prefix of the other. */
	if (old[shared] == 0 || new[shared] == 0) {
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
			fibril_rwlock_write_unlock(&namespace_rwlock);
			return rc;
		}
		
		base = vfs_node_get(&base_lr);
		old[shared] = '/';
		old += shared;
		new += shared;
	} else {
		vfs_node_addref(base);
	}
	
	
	rc = vfs_lookup_internal(base, new, L_UNLINK | L_DISABLE_MOUNTS, &new_lr_orig);
	if (rc == EOK) {
		orig_unlinked = true;
	} else if (rc != ENOENT) {
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}
	
	rc = vfs_lookup_internal(base, old, L_UNLINK | L_DISABLE_MOUNTS, &old_lr);
	if (rc != EOK) {
		if (orig_unlinked) {
			vfs_link_internal(base, new, &new_lr_orig.triplet);
		}
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}
	
	rc = vfs_link_internal(base, new, &old_lr.triplet);
	if (rc != EOK) {
		vfs_link_internal(base, old, &old_lr.triplet);
		if (orig_unlinked) {
			vfs_link_internal(base, new, &new_lr_orig.triplet);
		}
		vfs_node_put(base);
		fibril_rwlock_write_unlock(&namespace_rwlock);
		return rc;
	}
	
	/* If the node is not held by anyone, try to destroy it. */
	if (orig_unlinked && vfs_node_peek(&new_lr_orig) == NULL) {
		out_destroy(&new_lr_orig.triplet);
	}
	
	vfs_node_put(base);
	fibril_rwlock_write_unlock(&namespace_rwlock);
	return EOK;
}

void vfs_rename(ipc_callid_t rid, ipc_call_t *request)
{
	/* The common base directory. */
	int basefd;
	char *old = NULL;
	char *new = NULL;
	vfs_file_t *base = NULL;
	int rc;
	
	basefd = IPC_GET_ARG1(*request);
	
	/* Retrieve the old path. */
	rc = async_data_write_accept((void **) &old, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		goto out;
	}
	
	/* Retrieve the new path. */
	rc = async_data_write_accept((void **) &new, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		goto out;
	}
	
	size_t olen;
	size_t nlen;
	char *oldc = canonify(old, &olen);
	char *newc = canonify(new, &nlen);
	
	if ((!oldc) || (!newc)) {
		rc = EINVAL;
		goto out;
	}
	
	assert(oldc[olen] == '\0');
	assert(newc[nlen] == '\0');
	
	/* Lookup the file structure corresponding to the file descriptor. */
	vfs_node_t *base_node = root;
	// TODO: Client-side root.
	if (basefd != -1) {
		base = vfs_file_get(basefd);
		if (!base) {
			rc = EBADF;
			goto out;
		}
		base_node = base->node;
	}
	
	rc = vfs_rename_internal(base_node, oldc, newc);

out:
	async_answer_0(rid, rc);

	if (old) {
		free(old);
	}
	if (new) {
		free(new);
	}
	if (base) {
		vfs_file_put(base);
	}
}

void vfs_dup(ipc_callid_t rid, ipc_call_t *request)
{
	int oldfd = IPC_GET_ARG1(*request);
	int newfd = IPC_GET_ARG2(*request);
	
	/* If the file descriptors are the same, do nothing. */
	if (oldfd == newfd) {
		async_answer_1(rid, EOK, newfd);
		return;
	}
	
	/* Lookup the file structure corresponding to oldfd. */
	vfs_file_t *oldfile = vfs_file_get(oldfd);
	if (!oldfile) {
		async_answer_0(rid, EBADF);
		return;
	}
	
	/* Make sure newfd is closed. */
	(void) vfs_fd_free(newfd);
	
	/* Assign the old file to newfd. */
	int ret = vfs_fd_assign(oldfile, newfd);
	vfs_file_put(oldfile);
	
	if (ret != EOK)
		async_answer_0(rid, ret);
	else
		async_answer_1(rid, EOK, newfd);
}

void vfs_wait_handle(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = vfs_wait_handle_internal();
	async_answer_1(rid, EOK, fd);
}

void vfs_get_mtab(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	ipc_call_t data;
	sysarg_t rc = EOK;
	size_t len;

	fibril_mutex_lock(&mtab_list_lock);

	/* Send to the caller the number of mounted filesystems */
	callid = async_get_call(&data);
	if (IPC_GET_IMETHOD(data) != VFS_IN_PING) {
		rc = ENOTSUP;
		async_answer_0(callid, rc);
		goto exit;
	}
	async_answer_1(callid, EOK, mtab_size);

	list_foreach(mtab_list, link, mtab_ent_t, mtab_ent) {
		rc = ENOTSUP;

		if (!async_data_read_receive(&callid, &len)) {
			async_answer_0(callid, rc);
			goto exit;
		}

		(void) async_data_read_finalize(callid, mtab_ent->mp,
		    str_size(mtab_ent->mp));

		if (!async_data_read_receive(&callid, &len)) {
			async_answer_0(callid, rc);
			goto exit;
		}

		(void) async_data_read_finalize(callid, mtab_ent->opts,
		    str_size(mtab_ent->opts));

		if (!async_data_read_receive(&callid, &len)) {
			async_answer_0(callid, rc);
			goto exit;
		}

		(void) async_data_read_finalize(callid, mtab_ent->fs_name,
		    str_size(mtab_ent->fs_name));

		callid = async_get_call(&data);

		if (IPC_GET_IMETHOD(data) != VFS_IN_PING) {
			async_answer_0(callid, rc);
			goto exit;
		}

		rc = EOK;
		async_answer_2(callid, rc, mtab_ent->instance,
		    mtab_ent->service_id);
	}

exit:
	fibril_mutex_unlock(&mtab_list_lock);
	async_answer_0(rid, rc);
}

void vfs_statfs(ipc_callid_t rid, ipc_call_t *request)
{
	char *path;
	int rc = async_data_write_accept((void **) &path, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	
	ipc_callid_t callid;
	if (!async_data_read_receive(&callid, NULL)) {
		free(path);
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	vfs_lookup_res_t lr;
	fibril_rwlock_read_lock(&namespace_rwlock);
	rc = vfs_lookup_internal(root, path, L_NONE, &lr);
	free(path);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		async_answer_0(callid, rc);
		async_answer_0(rid, rc);
		return;
	}
	vfs_node_t *node = vfs_node_get(&lr);
	if (!node) {
		fibril_rwlock_read_unlock(&namespace_rwlock);
		async_answer_0(callid, ENOMEM);
		async_answer_0(rid, ENOMEM);
		return;
	}

	fibril_rwlock_read_unlock(&namespace_rwlock);

	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
	
	aid_t msg;
	msg = async_send_3(exch, VFS_OUT_STATFS, node->service_id,
	    node->index, false, NULL);
	async_forward_fast(callid, exch, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
	
	vfs_exchange_release(exch);
	
	sysarg_t rv;
	async_wait_for(msg, &rv);

	async_answer_0(rid, rv);

	vfs_node_put(node);
}

/**
 * @}
 */
