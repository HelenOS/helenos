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

/** @addtogroup fs
 * @{
 */

/**
 * @file vfs.c
 * @brief VFS service for HelenOS.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <bool.h>
#include <string.h>
#include <as.h>
#include <atomic.h>
#include "vfs.h"

#define NAME "vfs"

static void vfs_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	bool keep_on_going = true;

	/*
	 * The connection was opened via the IPC_CONNECT_ME_TO call.
	 * This call needs to be answered.
	 */
	ipc_answer_0(iid, EOK);
	
	/*
	 * Here we enter the main connection fibril loop.
	 * The logic behind this loop and the protocol is that we'd like to keep
	 * each connection open until the client hangs up. When the client hangs
	 * up, we will free its VFS state. The act of hanging up the connection
	 * by the client is equivalent to client termination because we cannot
	 * distinguish one from the other. On the other hand, the client can
	 * hang up arbitrarily if it has no open files and reestablish the
	 * connection later.
	 */
	while (keep_on_going) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		fs_handle_t fs_handle;
		int phone;
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			keep_on_going = false;
			break;
		case VFS_IN_REGISTER:
			vfs_register(callid, &call);
			keep_on_going = false;
			break;
		case VFS_IN_MOUNT:
			vfs_mount(callid, &call);
			break;
		case VFS_IN_OPEN:
			vfs_open(callid, &call);
			break;
		case VFS_IN_OPEN_NODE:
			vfs_open_node(callid, &call);
			break;
		case VFS_IN_CLOSE:
			vfs_close(callid, &call);
			break;
		case VFS_IN_READ:
			vfs_read(callid, &call);
			break;
		case VFS_IN_WRITE:
			vfs_write(callid, &call);
			break;
		case VFS_IN_SEEK:
			vfs_seek(callid, &call);
			break;
		case VFS_IN_TRUNCATE:
			vfs_truncate(callid, &call);
			break;
		case VFS_IN_MKDIR:
			vfs_mkdir(callid, &call);
			break;
		case VFS_IN_UNLINK:
			vfs_unlink(callid, &call);
			break;
		case VFS_IN_RENAME:
			vfs_rename(callid, &call);
			break;
		case VFS_IN_DEVICE:
			vfs_device(callid, &call);
			break;
		case VFS_IN_SYNC:
			vfs_sync(callid, &call);
			break;
		case VFS_IN_NODE:
			vfs_node(callid, &call);
			break;
		default:
			ipc_answer_0(callid, ENOTSUP);
			break;
		}
	}
	
	/* TODO: cleanup after the client */
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS VFS server\n");
	
	/*
	 * Initialize VFS node hash table.
	 */
	if (!vfs_nodes_init()) {
		printf(NAME ": Failed to initialize VFS node hash table\n");
		return ENOMEM;
	}
	
	/*
	 * Allocate and initialize the Path Lookup Buffer.
	 */
	plb = as_get_mappable_page(PLB_SIZE);
	if (!plb) {
		printf(NAME ": Cannot allocate a mappable piece of address space\n");
		return ENOMEM;
	}
	
	if (as_area_create(plb, PLB_SIZE, AS_AREA_READ | AS_AREA_WRITE |
	    AS_AREA_CACHEABLE) != plb) {
		printf(NAME ": Cannot create address space area\n");
		return ENOMEM;
	}
	memset(plb, 0, PLB_SIZE);
	
	/*
	 * Set a connection handling function/fibril.
	 */
	async_set_client_connection(vfs_connection);

	/*
	 * Register at the naming service.
	 */
	ipcarg_t phonead;
	ipc_connect_to_me(PHONE_NS, SERVICE_VFS, 0, 0, &phonead);
	
	/*
	 * Start accepting connections.
	 */
	printf(NAME ": Accepting connections\n");
	async_manager();
	return 0;
}

/**
 * @}
 */
