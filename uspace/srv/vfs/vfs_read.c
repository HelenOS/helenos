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
 * @file	vfs_read.c
 * @brief
 */

#include "vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>

void vfs_read(ipc_callid_t rid, ipc_call_t *request)
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

	/*
	 * Because we don't support the receive analogy of IPC_M_DATA_SEND,
	 * VFS_READ is emulutating its behavior via sharing an address space
	 * area.
	 */

	int fd = IPC_GET_ARG1(*request);
	size_t size = IPC_GET_ARG2(*request);

	/*
	 * Lookup the file structure corresponding to the file descriptor.
	 */
	vfs_file_t *file = vfs_file_get(fd);
	if (!file) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/*
	 * Now we need to receive a call with client's address space area.
	 */
	ipc_callid_t callid;
	ipc_call_t call;
	callid = async_get_call(&call);
	if (IPC_GET_METHOD(call) != IPC_M_AS_AREA_SEND) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	int fs_phone = vfs_grab_phone(file->node->fs_handle);	
	
	/*
	 * Make a VFS_READ request at the destination FS server.
	 */
	aid_t msg;
	ipc_call_t answer;
	msg = async_send_4(fs_phone, VFS_READ, file->node->dev_handle,
	    file->node->index, file->pos, size, &answer);
	
	/*
	 * Forward the address space area offer to the destination FS server.
	 * The call will be routed as if sent by ourselves.
	 */
	ipc_forward_fast(callid, fs_phone, IPC_GET_METHOD(call),
	    IPC_GET_ARG1(call), 0, IPC_FF_ROUTE_FROM_ME);

	vfs_release_phone(fs_phone);

	/*
	 * Wait for reply from the FS server.
	 */
	ipcarg_t rc;
	async_wait_for(msg, &rc);
	size_t bytes = IPC_GET_ARG1(answer);

	/*
	 * FS server's reply is the final result of the whole operation we
	 * return to the client.
	 */
	ipc_answer_1(rid, rc, bytes);
}

/**
 * @}
 */ 
