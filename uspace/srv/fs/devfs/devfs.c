/*
 * Copyright (c) 2009 Martin Decky
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
 * @file devfs.c
 * @brief Devices file system.
 *
 * Every device registered to device mapper is represented as a file in this
 * file system.
 */

#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <libfs.h>
#include "devfs.h"
#include "devfs_ops.h"

#define NAME  "devfs"

static vfs_info_t devfs_vfs_info = {
	.name = "devfs",
};

fs_reg_t devfs_reg;

static void devfs_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	if (iid)
		ipc_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch  (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			return;
		case VFS_MOUNTED:
			devfs_mounted(callid, &call);
			break;
		case VFS_MOUNT:
			devfs_mount(callid, &call);
			break;
		case VFS_LOOKUP:
			devfs_lookup(callid, &call);
			break;
		case VFS_OPEN_NODE:
			devfs_open_node(callid, &call);
			break;
		case VFS_DEVICE:
			devfs_device(callid, &call);
			break;
		case VFS_READ:
			devfs_read(callid, &call);
			break;
		case VFS_WRITE:
			devfs_write(callid, &call);
			break;
		case VFS_TRUNCATE:
			devfs_truncate(callid, &call);
			break;
		case VFS_CLOSE:
			devfs_close(callid, &call);
			break;
		case VFS_SYNC:
			devfs_sync(callid, &call);
			break;
		case VFS_DESTROY:
			devfs_destroy(callid, &call);
			break;
		default:
			ipc_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Filesystem\n");
	
	if (!devfs_init()) {
		printf(NAME ": failed to initialize devfs\n");
		return -1;
	}
	
	int vfs_phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_VFS, 0, 0);
	if (vfs_phone < EOK) {
		printf(NAME ": Unable to connect to VFS\n");
		return -1;
	}
	
	int rc = fs_register(vfs_phone, &devfs_reg, &devfs_vfs_info,
	    devfs_connection);
	if (rc != EOK) {
		printf(NAME ": Failed to register file system (%d)\n", rc);
		return rc;
	}
	
	printf(NAME ": Accepting connections\n");
	async_manager();
	
	/* Not reached */
	return 0;
}

/**
 * @}
 */
