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
 * @file	vfs_rdwr.c
 * @brief
 */

#include "vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <rwlock.h>
#include <unistd.h>

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
		rwlock_reader_lock(&file->node->contents_rwlock);
	else
		rwlock_writer_lock(&file->node->contents_rwlock);

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
		rwlock_reader_unlock(&file->node->contents_rwlock);
	else
		rwlock_writer_unlock(&file->node->contents_rwlock);

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
		rwlock_reader_lock(&file->node->contents_rwlock);
		size_t size = file->node->size;
		rwlock_reader_unlock(&file->node->contents_rwlock);
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

/**
 * @}
 */ 
