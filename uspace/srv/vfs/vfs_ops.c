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
#include <ipc/services.h>
#include <async.h>
#include <fibril.h>
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
#include <as.h>
#include <assert.h>
#include <atomic.h>
#include "vfs.h"

#define min(a, b)	((a) < (b) ? (a) : (b))

/**
 * This rwlock prevents the race between a triplet-to-VFS-node resolution and a
 * concurrent VFS operation which modifies the file system namespace.
 */
RWLOCK_INITIALIZE(namespace_rwlock);

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

atomic_t rootfs_futex = FUTEX_INITIALIZER;
vfs_triplet_t rootfs = {
	.fs_handle = 0,
	.dev_handle = 0,
	.index = 0,
};

static int lookup_root(int fs_handle, int dev_handle, vfs_triplet_t *root,
    size_t *size)
{
	vfs_pair_t altroot = {
		.fs_handle = fs_handle,
		.dev_handle = dev_handle,
	};

	return vfs_lookup_internal("/", strlen("/"), root, size, &altroot);
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

	/*
	 * Deliver the file system name.
	 */
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

	/*
	 * Now, we want the client to send us the mount point.
	 */
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Check whether size is reasonable wrt. the mount point.
	 */
	if (size < 1 || size > MAX_PATH_LEN) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	/*
	 * Allocate buffer for the mount point data being received.
	 */
	uint8_t *buf;
	buf = malloc(size);
	if (!buf) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Deliver the mount point.
	 */
	(void) ipc_data_write_finalize(callid, buf, size);

	/*
	 * Lookup the root node of the filesystem being mounted.
	 * In this case, we don't need to take the namespace_futex as the root
	 * node cannot be removed. However, we do take a reference to it so
	 * that we can track how many times it has been mounted.
	 */
	int rc;
	vfs_triplet_t mounted_root;
	size_t mrsz;
	rc = lookup_root(fs_handle, dev_handle, &mounted_root, &mrsz);
	if (rc != EOK) {
		free(buf);
		ipc_answer_0(rid, rc);
		return;
	}
	vfs_node_t *mr_node = vfs_node_get(&mounted_root, mrsz);
	if (!mr_node) {
		free(buf);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	/*
	 * Finally, we need to resolve the path to the mountpoint. 
	 */
	vfs_triplet_t mp;
	size_t mpsz;
	futex_down(&rootfs_futex);
	if (rootfs.fs_handle) {
		/*
		 * We already have the root FS.
		 */
		rwlock_write_lock(&namespace_rwlock);
		rc = vfs_lookup_internal(buf, size, &mp, &mpsz, NULL);
		if (rc != EOK) {
			/*
			 * The lookup failed for some reason.
			 */
			rwlock_write_unlock(&namespace_rwlock);
			futex_up(&rootfs_futex);
			vfs_node_put(mr_node);	/* failed -> drop reference */
			free(buf);
			ipc_answer_0(rid, rc);
			return;
		}
		mp_node = vfs_node_get(&mp, mpsz);
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
		/*
		 * We still don't have the root file system mounted.
		 */
		if ((size == 1) && (buf[0] == '/')) {
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
	 * The POSIX interface is open(path, flags, mode).
	 * We can receive flags and mode along with the VFS_OPEN call; the path
	 * will need to arrive in another call.
	 */
	int flags = IPC_GET_ARG1(*request);
	int mode = IPC_GET_ARG2(*request);
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
	
	/*
	 * Avoid the race condition in which the file can be deleted before we
	 * find/create-and-lock the VFS node corresponding to the looked-up
	 * triplet.
	 */
	rwlock_read_lock(&namespace_rwlock);

	/*
	 * The path is now populated and we can call vfs_lookup_internal().
	 */
	vfs_triplet_t triplet;
	size_t size;
	rc = vfs_lookup_internal(path, len, &triplet, &size, NULL);
	if (rc) {
		rwlock_read_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}

	/*
	 * Path is no longer needed.
	 */
	free(path);

	vfs_node_t *node = vfs_node_get(&triplet, size);
	rwlock_read_unlock(&namespace_rwlock);

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

	/*
	 * The following increase in reference count is for the fact that the
	 * file is being opened and that a file structure is pointing to it.
	 * It is necessary so that the file will not disappear when
	 * vfs_node_put() is called. The reference will be dropped by the
	 * respective VFS_CLOSE.
	 */
	vfs_node_addref(node);
	vfs_node_put(node);

	/*
	 * Success! Return the new file descriptor to the client.
	 */
	ipc_answer_1(rid, EOK, fd);
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

	/*
	 * Lookup the file structure corresponding to the file descriptor.
	 */
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
	
	/*
	 * Make a VFS_READ/VFS_WRITE request at the destination FS server.
	 */
	aid_t msg;
	ipc_call_t answer;
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

	/*
	 * Wait for reply from the FS server.
	 */
	ipcarg_t rc;
	async_wait_for(msg, &rc);
	size_t bytes = IPC_GET_ARG1(answer);

	/*
	 * Unlock the VFS node.
	 */
	if (read)
		rwlock_read_unlock(&file->node->contents_rwlock);
	else {
		/* Update the cached version of node's size. */
		file->node->size = IPC_GET_ARG2(answer); 
		rwlock_write_unlock(&file->node->contents_rwlock);
	}

	/*
	 * Update the position pointer and unlock the open file.
	 */
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


	/*
	 * Lookup the file structure corresponding to the file descriptor.
	 */
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

atomic_t fs_head_futex = FUTEX_INITIALIZER;
link_t fs_head;

atomic_t fs_handle_next = {
	.count = 1
};

/** Verify the VFS info structure.
 *
 * @param info		Info structure to be verified.
 *
 * @return		Non-zero if the info structure is sane, zero otherwise.
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
	

	/*
	 * Check if the FS implements mandatory VFS operations.
	 */
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_REGISTER)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_REGISTER not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_MOUNT)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_MOUNT not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_UNMOUNT)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_UNMOUNT not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_LOOKUP)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_LOOKUP not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_OPEN)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_OPEN not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_CLOSE)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_CLOSE not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_READ)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_READ not defined by the client.\n");
		return false;
	}
	
	/*
	 * Check if each operation is either not defined, defined or default.
	 */
	for (i = VFS_FIRST; i < VFS_LAST; i++) {
		if ((info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_NULL) && 
		    (info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_DEFAULT) && 
		    (info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_DEFINED)) {
			dprintf("Operation info not understood.\n");
			return false;
		}
	}
	return true;
}

/** VFS_REGISTER protocol function.
 *
 * @param rid		Hash of the call with the request.
 * @param request	Call structure with the request.
 */
void vfs_register(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int rc;
	size_t size;

	dprintf("Processing VFS_REGISTER request received from %p.\n",
	    request->in_phone_hash);

	/*
	 * The first call has to be IPC_M_DATA_SEND in which we receive the
	 * VFS info structure from the client FS.
	 */
	if (!ipc_data_write_receive(&callid, &size)) {
		/*
		 * The client doesn't obey the same protocol as we do.
		 */
		dprintf("Receiving of VFS info failed.\n");
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	dprintf("VFS info received, size = %d\n", size);
	
	/*
	 * We know the size of the VFS info structure. See if the client
	 * understands this easy concept too.
	 */
	if (size != sizeof(vfs_info_t)) {
		/*
		 * The client is sending us something, which cannot be
		 * the info structure.
		 */
		dprintf("Received VFS info has bad size.\n");
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Allocate and initialize a buffer for the fs_info structure.
	 */
	fs_info_t *fs_info;
	fs_info = (fs_info_t *) malloc(sizeof(fs_info_t));
	if (!fs_info) {
		dprintf("Could not allocate memory for FS info.\n");
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	link_initialize(&fs_info->fs_link);
	futex_initialize(&fs_info->phone_futex, 1);
		
	rc = ipc_data_write_finalize(callid, &fs_info->vfs_info, size);
	if (rc != EOK) {
		dprintf("Failed to deliver the VFS info into our AS, rc=%d.\n",
		    rc);
		free(fs_info);
		ipc_answer_0(callid, rc);
		ipc_answer_0(rid, rc);
		return;
	}

	dprintf("VFS info delivered.\n");
		
	if (!vfs_info_sane(&fs_info->vfs_info)) {
		free(fs_info);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
		
	futex_down(&fs_head_futex);

	/*
	 * Check for duplicit registrations.
	 */
	if (fs_name_to_handle(fs_info->vfs_info.name, false)) {
		/*
		 * We already register a fs like this.
		 */
		dprintf("FS is already registered.\n");
		futex_up(&fs_head_futex);
		free(fs_info);
		ipc_answer_0(callid, EEXISTS);
		ipc_answer_0(rid, EEXISTS);
		return;
	}

	/*
	 * Add fs_info to the list of registered FS's.
	 */
	dprintf("Inserting FS into the list of registered file systems.\n");
	list_append(&fs_info->fs_link, &fs_head);

	/*
	 * Now we want the client to send us the IPC_M_CONNECT_TO_ME call so
	 * that a callback connection is created and we have a phone through
	 * which to forward VFS requests to it.
	 */
	callid = async_get_call(&call);
	if (IPC_GET_METHOD(call) != IPC_M_CONNECT_TO_ME) {
		dprintf("Unexpected call, method = %d\n", IPC_GET_METHOD(call));
		list_remove(&fs_info->fs_link);
		futex_up(&fs_head_futex);
		free(fs_info);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	fs_info->phone = IPC_GET_ARG5(call);
	ipc_answer_0(callid, EOK);

	dprintf("Callback connection to FS created.\n");

	/*
	 * The client will want us to send him the address space area with PLB.
	 */

	if (!ipc_share_in_receive(&callid, &size)) {
		dprintf("Unexpected call, method = %d\n", IPC_GET_METHOD(call));
		list_remove(&fs_info->fs_link);
		futex_up(&fs_head_futex);
		ipc_hangup(fs_info->phone);
		free(fs_info);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	/*
	 * We can only send the client address space area PLB_SIZE bytes long.
	 */
	if (size != PLB_SIZE) {
		dprintf("Client suggests wrong size of PFB, size = %d\n", size);
		list_remove(&fs_info->fs_link);
		futex_up(&fs_head_futex);
		ipc_hangup(fs_info->phone);
		free(fs_info);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Commit to read-only sharing the PLB with the client.
	 */
	(void) ipc_share_in_finalize(callid, plb,
	    AS_AREA_READ | AS_AREA_CACHEABLE);

	dprintf("Sharing PLB.\n");

	/*
	 * That was it. The FS has been registered.
	 * In reply to the VFS_REGISTER request, we assign the client file
	 * system a global file system handle.
	 */
	fs_info->fs_handle = (int) atomic_postinc(&fs_handle_next);
	ipc_answer_1(rid, EOK, (ipcarg_t) fs_info->fs_handle);
	
	futex_up(&fs_head_futex);
	
	dprintf("\"%.*s\" filesystem successfully registered, handle=%d.\n",
	    FS_NAME_MAXLEN, fs_info->vfs_info.name, fs_info->fs_handle);
}

/** For a given file system handle, implement policy for allocating a phone.
 *
 * @param handle	File system handle.
 *
 * @return		Phone over which a multi-call request can be safely
 *			sent. Return 0 if no phone was found.
 */
int vfs_grab_phone(int handle)
{
	/*
	 * For now, we don't try to be very clever and very fast.
	 * We simply lookup the phone in the fs_head list. We currently don't
	 * open any additional phones (even though that itself would be pretty
	 * straightforward; housekeeping multiple open phones to a FS task would
	 * be more demanding). Instead, we simply take the respective
	 * phone_futex and keep it until vfs_release_phone().
	 */
	futex_down(&fs_head_futex);
	link_t *cur;
	fs_info_t *fs;
	for (cur = fs_head.next; cur != &fs_head; cur = cur->next) {
		fs = list_get_instance(cur, fs_info_t, fs_link);
		if (fs->fs_handle == handle) {
			futex_up(&fs_head_futex);
			/*
			 * For now, take the futex unconditionally.
			 * Oh yeah, serialization rocks.
			 * It will be up'ed in vfs_release_phone().
			 */
			futex_down(&fs->phone_futex);
			/*
			 * Avoid deadlock with other fibrils in the same thread
			 * by disabling fibril preemption.
			 */
			fibril_inc_sercount();
			return fs->phone; 
		}
	}
	futex_up(&fs_head_futex);
	return 0;
}

/** Tell VFS that the phone is in use for any request.
 *
 * @param phone		Phone to FS task.
 */
void vfs_release_phone(int phone)
{
	bool found = false;

	/*
	 * Undo the fibril_inc_sercount() done in vfs_grab_phone().
	 */
	fibril_dec_sercount();
	
	futex_down(&fs_head_futex);
	link_t *cur;
	for (cur = fs_head.next; cur != &fs_head; cur = cur->next) {
		fs_info_t *fs = list_get_instance(cur, fs_info_t, fs_link);
		if (fs->phone == phone) {
			found = true;
			futex_up(&fs_head_futex);
			futex_up(&fs->phone_futex);
			return;
		}
	}
	futex_up(&fs_head_futex);

	/*
	 * Not good to get here.
	 */
	assert(found == true);
}

/** Convert file system name to its handle.
 *
 * @param name		File system name.
 * @param lock		If true, the function will down and up the
 * 			fs_head_futex.
 *
 * @return		File system handle or zero if file system not found.
 */
int fs_name_to_handle(char *name, bool lock)
{
	int handle = 0;
	
	if (lock)
		futex_down(&fs_head_futex);
	link_t *cur;
	for (cur = fs_head.next; cur != &fs_head; cur = cur->next) {
		fs_info_t *fs = list_get_instance(cur, fs_info_t, fs_link);
		if (strncmp(fs->vfs_info.name, name,
		    sizeof(fs->vfs_info.name)) == 0) { 
			handle = fs->fs_handle;
			break;
		}
	}
	if (lock)
		futex_up(&fs_head_futex);
	return handle;
}

/**
 * @}
 */ 
