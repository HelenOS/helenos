/*
 * Copyright (c) 2006 Martin Decky
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
 * @file	tmpfs.c
 * @brief	File system driver for in-memory file system.
 *
 * Every instance of tmpfs exists purely in memory and has neither a disk layout
 * nor any permanent storage (e.g. disk blocks). With each system reboot, data
 * stored in a tmpfs file system is lost.
 */

#include "tmpfs.h"
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <libfs.h>
#include "../../vfs/vfs.h"

#define NAME "tmpfs"


vfs_info_t tmpfs_vfs_info = {
	.name = "tmpfs",
};

fs_reg_t tmpfs_reg;

/**
 * This connection fibril processes VFS requests from VFS.
 *
 * In order to support simultaneous VFS requests, our design is as follows.
 * The connection fibril accepts VFS requests from VFS. If there is only one
 * instance of the fibril, VFS will need to serialize all VFS requests it sends
 * to FAT. To overcome this bottleneck, VFS can send TMPFS the
 * IPC_M_CONNECT_ME_TO call. In that case, a new connection fibril will be
 * created, which in turn will accept the call. Thus, a new phone will be
 * opened for VFS.
 *
 * There are few issues with this arrangement. First, VFS can run out of
 * available phones. In that case, VFS can close some other phones or use one
 * phone for more serialized requests. Similarily, TMPFS can refuse to duplicate
 * the connection. VFS should then just make use of already existing phones and
 * route its requests through them. To avoid paying the fibril creation price 
 * upon each request, TMPFS might want to keep the connections open after the
 * request has been completed.
 */
static void tmpfs_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	if (iid) {
		/*
		 * This only happens for connections opened by
		 * IPC_M_CONNECT_ME_TO calls as opposed to callback connections
		 * created by IPC_M_CONNECT_TO_ME.
		 */
		ipc_answer_0(iid, EOK);
	}
	
	dprintf("VFS-TMPFS connection established.\n");
	while (1) {
		ipc_callid_t callid;
		ipc_call_t call;
	
		callid = async_get_call(&call);
		switch  (IPC_GET_METHOD(call)) {
		case VFS_MOUNTED:
			tmpfs_mounted(callid, &call);
			break;
		case VFS_MOUNT:
			tmpfs_mount(callid, &call);
			break;
		case VFS_LOOKUP:
			tmpfs_lookup(callid, &call);
			break;
		case VFS_READ:
			tmpfs_read(callid, &call);
			break;
		case VFS_WRITE:
			tmpfs_write(callid, &call);
			break;
		case VFS_TRUNCATE:
			tmpfs_truncate(callid, &call);
			break;
		case VFS_DESTROY:
			tmpfs_destroy(callid, &call);
			break;
		default:
			ipc_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS TMPFS file system server\n");
	
	int vfs_phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_VFS, 0, 0);
	if (vfs_phone < EOK) {
		printf(NAME ": Unable to connect to VFS\n");
		return -1;
	}
	
	int rc = fs_register(vfs_phone, &tmpfs_reg, &tmpfs_vfs_info,
	    tmpfs_connection);
	if (rc != EOK) {
		printf(NAME ": Failed to register file system (%d)\n", rc);
		return rc;
	}
	
	printf(NAME ": Accepting connections\n");
	async_manager();
	/* not reached */
	return 0;
}

/**
 * @}
 */ 
