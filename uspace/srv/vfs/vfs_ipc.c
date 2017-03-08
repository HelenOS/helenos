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

#include "vfs.h"

#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/canonify.h>

static void vfs_in_clone(ipc_callid_t rid, ipc_call_t *request)
{
	int oldfd = IPC_GET_ARG1(*request);
	bool desc = IPC_GET_ARG2(*request);
	
	int ret = vfs_op_clone(oldfd, desc);
	async_answer_0(rid, ret);
}

static void vfs_in_close(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int rc = vfs_op_close(fd);
	async_answer_0(rid, rc);
}

static void vfs_in_dup(ipc_callid_t rid, ipc_call_t *request)
{
	int oldfd = IPC_GET_ARG1(*request);
	int newfd = IPC_GET_ARG2(*request);
	int rc = vfs_op_dup(oldfd, newfd);
	async_answer_1(rid, rc, newfd);
}

static void vfs_in_fstat(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int rc = vfs_op_fstat_forward(fd);
	async_answer_0(rid, rc);
}

static void vfs_in_mount(ipc_callid_t rid, ipc_call_t *request)
{
	int mpfd = IPC_GET_ARG1(*request);
	
	/*
	 * We expect the library to do the device-name to device-handle
	 * translation for us, thus the device handle will arrive as ARG1
	 * in the request.
	 */
	service_id_t service_id = (service_id_t) IPC_GET_ARG2(*request);
	
	unsigned int flags = (unsigned int) IPC_GET_ARG3(*request);
	unsigned int instance = IPC_GET_ARG4(*request);
	
	char *opts = NULL;
	char *fs_name = NULL;
	
	/* Now we expect to receive the mount options. */
	int rc = async_data_write_accept((void **) &opts, true, 0,
	    MAX_MNTOPTS_LEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	
	/* Now, we expect the client to send us data with the name of the file 
	 * system.
	 */
	rc = async_data_write_accept((void **) &fs_name, true, 0,
	    FS_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(opts);
		async_answer_0(rid, rc);
		return;
	}
	
	int outfd = 0;
	rc = vfs_op_mount(mpfd, service_id, flags, instance, opts, fs_name,
	     &outfd);
	async_answer_1(rid, rc, outfd);
	
	free(opts);
	free(fs_name);
}

static void vfs_in_mtab_get(ipc_callid_t rid, ipc_call_t *request)
{
	int rc = vfs_op_mtab_get();
	async_answer_0(rid, rc);
}

static void vfs_in_open2(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int flags = IPC_GET_ARG2(*request);

	int rc = vfs_op_open2(fd, flags);
	async_answer_0(rid, rc);
}

static void vfs_in_read(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);

	size_t bytes = 0;
	int rc = vfs_op_read(fd, &bytes);
	async_answer_1(rid, rc, bytes);
}

static void vfs_in_rename(ipc_callid_t rid, ipc_call_t *request)
{
	/* The common base directory. */
	int basefd;
	char *old = NULL;
	char *new = NULL;
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
	
	rc = vfs_op_rename(basefd, oldc, newc);

out:
	async_answer_0(rid, rc);

	if (old) {
		free(old);
	}
	if (new) {
		free(new);
	}
}

static void vfs_in_seek(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = (int) IPC_GET_ARG1(*request);
	int64_t off = (int64_t) MERGE_LOUP32(IPC_GET_ARG2(*request), IPC_GET_ARG3(*request));
	int whence = (int) IPC_GET_ARG4(*request);

	int64_t new_offset = 0;
	int rc = vfs_op_seek(fd, off, whence, &new_offset);
	async_answer_2(rid, rc, LOWER32(new_offset), UPPER32(new_offset));
}

static void vfs_in_statfs(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = (int) IPC_GET_ARG1(*request);
	
	int rc = vfs_op_statfs(fd);
	async_answer_0(rid, rc);
}

static void vfs_in_sync(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int rc = vfs_op_sync(fd);
	async_answer_0(rid, rc);
}

static void vfs_in_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);
	int64_t size = MERGE_LOUP32(IPC_GET_ARG2(*request), IPC_GET_ARG3(*request));
	int rc = vfs_op_truncate(fd, size);
	async_answer_0(rid, rc);
}

static void vfs_in_unlink2(ipc_callid_t rid, ipc_call_t *request)
{
	int parentfd = IPC_GET_ARG1(*request);
	int expectfd = IPC_GET_ARG2(*request);
	int wflag = IPC_GET_ARG3(*request);
	
	char *path;
	int rc = async_data_write_accept((void **) &path, true, 0, 0, 0, NULL);
	if (rc == EOK) {
		rc = vfs_op_unlink2(parentfd, expectfd, wflag, path);
	}
	
	async_answer_0(rid, rc);
}

static void vfs_in_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	int mpfd = IPC_GET_ARG1(*request);
	int rc = vfs_op_unmount(mpfd);
	async_answer_0(rid, rc);
}

static void vfs_in_wait_handle(ipc_callid_t rid, ipc_call_t *request)
{
	bool high_fd = IPC_GET_ARG1(*request);
	int fd = vfs_op_wait_handle(high_fd);
	async_answer_1(rid, EOK, fd);
}

static void vfs_in_walk(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * Parent is our relative root for file lookup.
	 * For defined flags, see <ipc/vfs.h>.
	 */
	int parentfd = IPC_GET_ARG1(*request);
	int flags = IPC_GET_ARG2(*request);
	
	int fd = 0;
	char *path;
	int rc = async_data_write_accept((void **)&path, true, 0, 0, 0, NULL);
	if (rc == EOK) {
		rc = vfs_op_walk(parentfd, flags, path, &fd);
		free(path);
	}
	async_answer_1(rid, rc, fd);
}

static void vfs_in_write(ipc_callid_t rid, ipc_call_t *request)
{
	int fd = IPC_GET_ARG1(*request);

	size_t bytes = 0;
	int rc = vfs_op_write(fd, &bytes);
	async_answer_1(rid, rc, bytes);
}

void vfs_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	bool cont = true;
	
	/*
	 * The connection was opened via the IPC_CONNECT_ME_TO call.
	 * This call needs to be answered.
	 */
	async_answer_0(iid, EOK);
	
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case VFS_IN_CLONE:
			vfs_in_clone(callid, &call);
			break;
		case VFS_IN_CLOSE:
			vfs_in_close(callid, &call);
			break;
		case VFS_IN_DUP:
			vfs_in_dup(callid, &call);
			break;
		case VFS_IN_FSTAT:
			vfs_in_fstat(callid, &call);
			break;
		case VFS_IN_MOUNT:
			vfs_in_mount(callid, &call);
			break;
		case VFS_IN_MTAB_GET:
			vfs_in_mtab_get(callid, &call);
			break;
		case VFS_IN_OPEN2:
			vfs_in_open2(callid, &call);
			break;
		case VFS_IN_READ:
			vfs_in_read(callid, &call);
			break;
		case VFS_IN_REGISTER:
			vfs_register(callid, &call);
			cont = false;
			break;
		case VFS_IN_RENAME:
			vfs_in_rename(callid, &call);
			break;
		case VFS_IN_SEEK:
			vfs_in_seek(callid, &call);
			break;
		case VFS_IN_STATFS:
			vfs_in_statfs(callid, &call);
			break;
		case VFS_IN_SYNC:
			vfs_in_sync(callid, &call);
			break;
		case VFS_IN_TRUNCATE:
			vfs_in_truncate(callid, &call);
			break;
		case VFS_IN_UNLINK2:
			vfs_in_unlink2(callid, &call);
			break;
		case VFS_IN_UNMOUNT:
			vfs_in_unmount(callid, &call);
			break;
		case VFS_IN_WAIT_HANDLE:
			vfs_in_wait_handle(callid, &call);
			break;
		case VFS_IN_WALK:
			vfs_in_walk(callid, &call);
			break;
		case VFS_IN_WRITE:
			vfs_in_write(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
	
	/*
	 * Open files for this client will be cleaned up when its last
	 * connection fibril terminates.
	 */
}
