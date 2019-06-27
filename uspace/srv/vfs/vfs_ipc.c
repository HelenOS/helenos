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

#include <vfs/vfs.h>
#include "vfs.h"

#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/canonify.h>

static void vfs_in_clone(ipc_call_t *req)
{
	int oldfd = ipc_get_arg1(req);
	int newfd = ipc_get_arg2(req);
	bool desc = ipc_get_arg3(req);

	int outfd = -1;
	errno_t rc = vfs_op_clone(oldfd, newfd, desc, &outfd);
	async_answer_1(req, rc, outfd);
}

static void vfs_in_fsprobe(ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) ipc_get_arg1(req);
	char *fs_name = NULL;
	vfs_fs_probe_info_t info;
	errno_t rc;

	/*
	 * Now we expect the client to send us data with the name of the file
	 * system.
	 */
	rc = async_data_write_accept((void **) &fs_name, true, 0,
	    FS_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(req, rc);
		return;
	}

	rc = vfs_op_fsprobe(fs_name, service_id, &info);
	async_answer_0(req, rc);
	if (rc != EOK)
		goto out;

	/* Now we should get a read request */
	ipc_call_t call;
	size_t len;
	if (!async_data_read_receive(&call, &len))
		goto out;

	if (len > sizeof(info))
		len = sizeof(info);
	(void) async_data_read_finalize(&call, &info, len);

out:
	free(fs_name);
}

static void vfs_in_fstypes(ipc_call_t *req)
{
	vfs_fstypes_t fstypes;
	errno_t rc;

	rc = vfs_get_fstypes(&fstypes);
	if (rc != EOK) {
		async_answer_0(req, ENOMEM);
		return;
	}

	/* Send size of the data */
	async_answer_1(req, EOK, fstypes.size);

	/* Now we should get a read request */
	ipc_call_t call;
	size_t len;
	if (!async_data_read_receive(&call, &len))
		goto out;

	if (len > fstypes.size)
		len = fstypes.size;
	(void) async_data_read_finalize(&call, fstypes.buf, len);

out:
	vfs_fstypes_free(&fstypes);
}

static void vfs_in_mount(ipc_call_t *req)
{
	int mpfd = ipc_get_arg1(req);

	/*
	 * We expect the library to do the device-name to device-handle
	 * translation for us, thus the device handle will arrive as ARG1
	 * in the request.
	 */
	service_id_t service_id = (service_id_t) ipc_get_arg2(req);

	unsigned int flags = (unsigned int) ipc_get_arg3(req);
	unsigned int instance = ipc_get_arg4(req);

	char *opts = NULL;
	char *fs_name = NULL;

	/* Now we expect to receive the mount options. */
	errno_t rc = async_data_write_accept((void **) &opts, true, 0,
	    MAX_MNTOPTS_LEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(req, rc);
		return;
	}

	/*
	 * Now we expect the client to send us data with the name of the file
	 * system.
	 */
	rc = async_data_write_accept((void **) &fs_name, true, 0,
	    FS_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(opts);
		async_answer_0(req, rc);
		return;
	}

	int outfd = 0;
	rc = vfs_op_mount(mpfd, service_id, flags, instance, opts, fs_name,
	    &outfd);
	async_answer_1(req, rc, outfd);

	free(opts);
	free(fs_name);
}

static void vfs_in_open(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	int mode = ipc_get_arg2(req);

	errno_t rc = vfs_op_open(fd, mode);
	async_answer_0(req, rc);
}

static void vfs_in_put(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	errno_t rc = vfs_op_put(fd);
	async_answer_0(req, rc);
}

static void vfs_in_read(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	aoff64_t pos = MERGE_LOUP32(ipc_get_arg2(req),
	    ipc_get_arg3(req));

	size_t bytes = 0;
	errno_t rc = vfs_op_read(fd, pos, &bytes);
	async_answer_1(req, rc, bytes);
}

static void vfs_in_rename(ipc_call_t *req)
{
	/* The common base directory. */
	int basefd;
	char *old = NULL;
	char *new = NULL;
	errno_t rc;

	basefd = ipc_get_arg1(req);

	/* Retrieve the old path. */
	rc = async_data_write_accept((void **) &old, true, 0, 0, 0, NULL);
	if (rc != EOK)
		goto out;

	/* Retrieve the new path. */
	rc = async_data_write_accept((void **) &new, true, 0, 0, 0, NULL);
	if (rc != EOK)
		goto out;

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
	async_answer_0(req, rc);

	if (old)
		free(old);
	if (new)
		free(new);
}

static void vfs_in_resize(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	int64_t size = MERGE_LOUP32(ipc_get_arg2(req), ipc_get_arg3(req));
	errno_t rc = vfs_op_resize(fd, size);
	async_answer_0(req, rc);
}

static void vfs_in_stat(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	errno_t rc = vfs_op_stat(fd);
	async_answer_0(req, rc);
}

static void vfs_in_statfs(ipc_call_t *req)
{
	int fd = (int) ipc_get_arg1(req);

	errno_t rc = vfs_op_statfs(fd);
	async_answer_0(req, rc);
}

static void vfs_in_sync(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	errno_t rc = vfs_op_sync(fd);
	async_answer_0(req, rc);
}

static void vfs_in_unlink(ipc_call_t *req)
{
	int parentfd = ipc_get_arg1(req);
	int expectfd = ipc_get_arg2(req);

	char *path;
	errno_t rc = async_data_write_accept((void **) &path, true, 0, 0, 0, NULL);
	if (rc == EOK)
		rc = vfs_op_unlink(parentfd, expectfd, path);

	async_answer_0(req, rc);
}

static void vfs_in_unmount(ipc_call_t *req)
{
	int mpfd = ipc_get_arg1(req);
	errno_t rc = vfs_op_unmount(mpfd);
	async_answer_0(req, rc);
}

static void vfs_in_wait_handle(ipc_call_t *req)
{
	bool high_fd = ipc_get_arg1(req);
	int fd = -1;
	errno_t rc = vfs_op_wait_handle(high_fd, &fd);
	async_answer_1(req, rc, fd);
}

static void vfs_in_walk(ipc_call_t *req)
{
	/*
	 * Parent is our relative root for file lookup.
	 * For defined flags, see <ipc/vfs.h>.
	 */
	int parentfd = ipc_get_arg1(req);
	int flags = ipc_get_arg2(req);

	int fd = 0;
	char *path;
	errno_t rc = async_data_write_accept((void **)&path, true, 0, 0, 0, NULL);
	if (rc == EOK) {
		rc = vfs_op_walk(parentfd, flags, path, &fd);
		free(path);
	}
	async_answer_1(req, rc, fd);
}

static void vfs_in_write(ipc_call_t *req)
{
	int fd = ipc_get_arg1(req);
	aoff64_t pos = MERGE_LOUP32(ipc_get_arg2(req),
	    ipc_get_arg3(req));

	size_t bytes = 0;
	errno_t rc = vfs_op_write(fd, pos, &bytes);
	async_answer_1(req, rc, bytes);
}

void vfs_connection(ipc_call_t *icall, void *arg)
{
	bool cont = true;

	/*
	 * The connection was opened via the IPC_CONNECT_ME_TO call.
	 * This call needs to be answered.
	 */
	async_accept_0(icall);

	while (cont) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case VFS_IN_CLONE:
			vfs_in_clone(&call);
			break;
		case VFS_IN_FSPROBE:
			vfs_in_fsprobe(&call);
			break;
		case VFS_IN_FSTYPES:
			vfs_in_fstypes(&call);
			break;
		case VFS_IN_MOUNT:
			vfs_in_mount(&call);
			break;
		case VFS_IN_OPEN:
			vfs_in_open(&call);
			break;
		case VFS_IN_PUT:
			vfs_in_put(&call);
			break;
		case VFS_IN_READ:
			vfs_in_read(&call);
			break;
		case VFS_IN_REGISTER:
			vfs_register(&call);
			cont = false;
			break;
		case VFS_IN_RENAME:
			vfs_in_rename(&call);
			break;
		case VFS_IN_RESIZE:
			vfs_in_resize(&call);
			break;
		case VFS_IN_STAT:
			vfs_in_stat(&call);
			break;
		case VFS_IN_STATFS:
			vfs_in_statfs(&call);
			break;
		case VFS_IN_SYNC:
			vfs_in_sync(&call);
			break;
		case VFS_IN_UNLINK:
			vfs_in_unlink(&call);
			break;
		case VFS_IN_UNMOUNT:
			vfs_in_unmount(&call);
			break;
		case VFS_IN_WAIT_HANDLE:
			vfs_in_wait_handle(&call);
			break;
		case VFS_IN_WALK:
			vfs_in_walk(&call);
			break;
		case VFS_IN_WRITE:
			vfs_in_write(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

	/*
	 * Open files for this client will be cleaned up when its last
	 * connection fibril terminates.
	 */
}
