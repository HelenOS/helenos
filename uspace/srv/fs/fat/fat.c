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
 * @file	fat.c
 * @brief	FAT file system driver for HelenOS.
 */

#include "fat.h"
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <libfs.h>
#include "../../vfs/vfs.h"

#define NAME	"fat"

vfs_info_t fat_vfs_info = {
	.name = NAME,
};

fs_reg_t fat_reg;

/**
 * This connection fibril processes VFS requests from VFS.
 *
 * In order to support simultaneous VFS requests, our design is as follows.
 * The connection fibril accepts VFS requests from VFS. If there is only one
 * instance of the fibril, VFS will need to serialize all VFS requests it sends
 * to FAT. To overcome this bottleneck, VFS can send FAT the IPC_M_CONNECT_ME_TO
 * call. In that case, a new connection fibril will be created, which in turn
 * will accept the call. Thus, a new phone will be opened for VFS.
 *
 * There are few issues with this arrangement. First, VFS can run out of
 * available phones. In that case, VFS can close some other phones or use one
 * phone for more serialized requests. Similarily, FAT can refuse to duplicate
 * the connection. VFS should then just make use of already existing phones and
 * route its requests through them. To avoid paying the fibril creation price 
 * upon each request, FAT might want to keep the connections open after the
 * request has been completed.
 */
static void fat_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	if (iid) {
		/*
		 * This only happens for connections opened by
		 * IPC_M_CONNECT_ME_TO calls as opposed to callback connections
		 * created by IPC_M_CONNECT_TO_ME.
		 */
		ipc_answer_0(iid, EOK);
	}
	
	dprintf(NAME ": connection opened\n");
	while (1) {
		ipc_callid_t callid;
		ipc_call_t call;
	
		callid = async_get_call(&call);
		switch  (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			return;
		case VFS_OUT_MOUNTED:
			fat_mounted(callid, &call);
			break;
		case VFS_OUT_MOUNT:
			fat_mount(callid, &call);
			break;
		case VFS_OUT_LOOKUP:
			fat_lookup(callid, &call);
			break;
		case VFS_OUT_READ:
			fat_read(callid, &call);
			break;
		case VFS_OUT_WRITE:
			fat_write(callid, &call);
			break;
		case VFS_OUT_TRUNCATE:
			fat_truncate(callid, &call);
			break;
		case VFS_OUT_STAT:
			fat_stat(callid, &call);
			break;
		case VFS_OUT_CLOSE:
			fat_close(callid, &call);
			break;
		case VFS_OUT_DESTROY:
			fat_destroy(callid, &call);
			break;
		case VFS_OUT_OPEN_NODE:
			fat_open_node(callid, &call);
			break;
		case VFS_OUT_SYNC:
			fat_sync(callid, &call);
			break;
		default:
			ipc_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	int vfs_phone;
	int rc;

	printf(NAME ": HelenOS FAT file system server\n");

	rc = fat_idx_init();
	if (rc != EOK)
		goto err;

	vfs_phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_VFS, 0, 0);
	if (vfs_phone < EOK) {
		printf(NAME ": failed to connect to VFS\n");
		return -1;
	}
	
	rc = fs_register(vfs_phone, &fat_reg, &fat_vfs_info, fat_connection);
	if (rc != EOK) {
		fat_idx_fini();
		goto err;
	}
	
	printf(NAME ": Accepting connections\n");
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
