/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Maurizio Lombardi
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
 * @file	mfs.c
 * @brief	Minix file system driver for HelenOS.
 */

#define _MAIN

#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdio.h>
#include "mfs.h"

vfs_info_t mfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,	
};


/**
 * This connection fibril processes VFS requests from VFS.
 *
 * In order to support simultaneous VFS requests, our design is as follows.
 * The connection fibril accepts VFS requests from VFS. If there is only one
 * instance of the fibril, VFS will need to serialize all VFS requests it sends
 * to MinixFS. To overcome this bottleneck, VFS can send MinixFS the IPC_M_CONNECT_ME_TO
 * call. In that case, a new connection fibril will be created, which in turn
 * will accept the call. Thus, a new phone will be opened for VFS.
 *
 * There are few issues with this arrangement. First, VFS can run out of
 * available phones. In that case, VFS can close some other phones or use one
 * phone for more serialized requests. Similarily, MinixFS can refuse to duplicate
 * the connection. VFS should then just make use of already existing phones and
 * route its requests through them. To avoid paying the fibril creation price 
 * upon each request, MinixFS might want to keep the connections open after the
 * request has been completed.
 */

static void mfs_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	if (iid) {
		/*
		 * This only happens for connections opened by
		 * IPC_M_CONNECT_ME_TO calls as opposed to callback connections
		 * created by IPC_M_CONNECT_TO_ME.
		 */
		async_answer_0(iid, EOK);
	}
	
	printf(NAME ": connection opened\n");
	while (1) {
		ipc_callid_t callid;
		ipc_call_t call;
	
		callid = async_get_call(&call);
		int method = IPC_GET_IMETHOD(call);

		mfsdebug(NAME "method = %d\n", method);
		switch  (method) {
		case IPC_M_PHONE_HUNGUP:
			return;
		case VFS_OUT_MOUNTED:
			mfsdebug("Mount request received\n");
			mfs_mounted(callid, &call);
			break;
		case VFS_OUT_MOUNT:
			mfs_mount(callid, &call);
			break;
		case VFS_OUT_STAT:
			mfs_stat(callid, &call);
			break;
		case VFS_OUT_LOOKUP:
			mfsdebug("lookup called\n");
			mfs_lookup(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
}


int main(int argc, char **argv)
{
	int vfs_phone;
	int rc;

	printf(NAME ": HelenOS Minix file system server\n");

	vfs_phone = service_connect_blocking(SERVICE_VFS, 0, 0);

	if (vfs_phone < EOK) {
		printf(NAME ": failed to connect to VFS\n");
		return -1;
	}

	rc = fs_register(vfs_phone, &mfs_reg, &mfs_vfs_info, mfs_connection);
	if (rc != EOK)
		goto err;
	
	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();
	/* not reached */
	return 0;

err:
	printf(NAME ": Failed to register file system (%d)\n", rc);
	return rc;
}

/**
 * @}
 */ 

