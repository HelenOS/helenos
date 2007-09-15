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
 * @file	vfs.c
 * @brief	VFS multiplexer for HelenOS.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <bool.h>
#include "vfs.h"

/** Verify the VFS info structure.
 *
 * @param info		Info structure to be verified.
 *
 * @return		Non-zero if the info structure is sane, zero otherwise.
 */
static int vfs_info_sane(vfs_info_t *info)
{
	return 1;	/* XXX */
}

/** VFS_REGISTER protocol function.
 *
 * @param rid		Hash of the call with the request.
 * @param request	Call structure with the request.
 */
static void vfs_register(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int rc;
	size_t size;

	/*
	 * The first call has to be IPC_M_DATA_SEND in which we receive the
	 * VFS info structure from the client FS.
	 */
	if (!ipc_data_send_accept(&callid, &call, NULL, &size)) {
		/*
		 * The client doesn't obey the same protocol as we do.
		 */ 
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
	
	/*
	 * We know the size of the info structure. See if the client understands
	 * this easy concept too.
	 */
	if (size != sizeof(vfs_info_t)) {
		/*
		 * The client is sending us something, which cannot be
		 * the info structure.
		 */
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
	vfs_info_t *info;

	/*
	 * Allocate a buffer for the info structure.
	 */
	info = (vfs_info_t *) malloc(sizeof(vfs_info_t));
	if (!info) {
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		ipc_answer_fast(rid, ENOMEM, 0, 0);
		return;
	}
		
	rc = ipc_data_send_answer(callid, &call, info, size);
	if (!rc) {
		free(info);
		ipc_answer_fast(callid, rc, 0, 0);
		ipc_answer_fast(rid, rc, 0, 0);
		return;
	}
		
	if (!vfs_info_sane(info)) {
		free(info);
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
		
}

static void vfs_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	bool keep_on_going = 1;

	/*
	 * The connection was opened via the IPC_CONNECT_ME_TO call.
	 * This call needs to be answered.
	 */
	ipc_answer_fast(iid, EOK, 0, 0);

	/*
	 * Here we enter the main connection fibril loop.
	 * The logic behind this loop and the protocol is that we'd like to keep
	 * each connection open for a while before we close it. The benefit of
	 * this is that the client doesn't have to establish a new connection
	 * upon each request.  On the other hand, the client must be ready to
	 * re-establish a connection if we hang it up due to reaching of maximum
	 * number of requests per connection or due to the client timing out.
	 */
	 
	while (keep_on_going) {
		ipc_callid_t callid;
		ipc_call_t call;

		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			keep_on_going = false;
			break;
		case VFS_REGISTER:
			vfs_register(callid, &call);
			keep_on_going = false;
			break;
		case VFS_MOUNT:
		case VFS_UNMOUNT:
		case VFS_OPEN:
		case VFS_CREATE:
		case VFS_CLOSE:
		case VFS_READ:
		case VFS_WRITE:
		case VFS_SEEK:
		default:
			ipc_answer_fast(callid, ENOTSUP, 0, 0);
			break;
		}
	}

	/* TODO: cleanup after the client */
	
}

int main(int argc, char **argv)
{
	ipcarg_t phonead;

	async_set_client_connection(vfs_connection);
	ipc_connect_to_me(PHONE_NS, SERVICE_VFS, 0, &phonead);
	async_manager();
	return 0;
}

/**
 * @}
 */ 
