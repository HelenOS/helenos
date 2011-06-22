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
 * @file locfs.c
 * @brief Location-service file system.
 *
 * Every service registered with location service is represented as a file in this
 * file system.
 */

#include <stdio.h>
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <task.h>
#include <libfs.h>
#include "locfs.h"
#include "locfs_ops.h"

#define NAME  "locfs"

static vfs_info_t locfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
};

fs_reg_t locfs_reg;

static void locfs_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	if (iid)
		async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			return;
		
		switch (IPC_GET_IMETHOD(call)) {
		case VFS_OUT_MOUNTED:
			locfs_mounted(callid, &call);
			break;
		case VFS_OUT_MOUNT:
			locfs_mount(callid, &call);
			break;
		case VFS_OUT_UNMOUNTED:
			locfs_unmounted(callid, &call);
			break;
		case VFS_OUT_UNMOUNT:
			locfs_unmount(callid, &call);
			break;
		case VFS_OUT_LOOKUP:
			locfs_lookup(callid, &call);
			break;
		case VFS_OUT_OPEN_NODE:
			locfs_open_node(callid, &call);
			break;
		case VFS_OUT_STAT:
			locfs_stat(callid, &call);
			break;
		case VFS_OUT_READ:
			locfs_read(callid, &call);
			break;
		case VFS_OUT_WRITE:
			locfs_write(callid, &call);
			break;
		case VFS_OUT_TRUNCATE:
			locfs_truncate(callid, &call);
			break;
		case VFS_OUT_CLOSE:
			locfs_close(callid, &call);
			break;
		case VFS_OUT_SYNC:
			locfs_sync(callid, &call);
			break;
		case VFS_OUT_DESTROY:
			locfs_destroy(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Device Filesystem\n", NAME);
	
	if (!locfs_init()) {
		printf("%s: failed to initialize locfs\n", NAME);
		return -1;
	}
	
	async_sess_t *vfs_sess = service_connect_blocking(EXCHANGE_SERIALIZE,
	    SERVICE_VFS, 0, 0);
	if (!vfs_sess) {
		printf("%s: Unable to connect to VFS\n", NAME);
		return -1;
	}
	
	int rc = fs_register(vfs_sess, &locfs_reg, &locfs_vfs_info,
	    locfs_connection);
	if (rc != EOK) {
		printf("%s: Failed to register file system (%d)\n", NAME, rc);
		return rc;
	}
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Not reached */
	return 0;
}

/**
 * @}
 */
