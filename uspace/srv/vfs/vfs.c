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

/** @addtogroup vfs
 * @{
 */

/**
 * @file vfs.c
 * @brief VFS service for HelenOS.
 */

#include <vfs/vfs.h>
#include <stdlib.h>
#include <ipc/services.h>
#include <abi/ipc/methods.h>
#include <libarch/config.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <stdbool.h>
#include <str.h>
#include <as.h>
#include <macros.h>
#include "vfs.h"

#define NAME  "vfs"

static void vfs_pager(ipc_call_t *icall, void *arg)
{
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case IPC_M_PAGE_IN:
			vfs_page_in(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}
}

static void notification_handler(ipc_call_t *call, void *arg)
{
	if (ipc_get_arg1(call) == VFS_PASS_HANDLE)
		vfs_op_pass_handle(
		    (task_id_t) MERGE_LOUP32(ipc_get_arg4(call),
		    ipc_get_arg5(call)), call->task_id,
		    (int) ipc_get_arg2(call));
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS VFS server\n", NAME);

	/*
	 * Initialize VFS node hash table.
	 */
	if (!vfs_nodes_init()) {
		printf("%s: Failed to initialize VFS node hash table\n",
		    NAME);
		return ENOMEM;
	}

	/*
	 * Allocate and initialize the Path Lookup Buffer.
	 */
	plb = as_area_create(AS_AREA_ANY, PLB_SIZE,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (plb == AS_MAP_FAILED) {
		printf("%s: Cannot create address space area\n", NAME);
		return ENOMEM;
	}
	memset(plb, 0, PLB_SIZE);

	/*
	 * Set client data constructor and destructor.
	 */
	async_set_client_data_constructor(vfs_client_data_create);
	async_set_client_data_destructor(vfs_client_data_destroy);

	/*
	 * Subscribe to notifications.
	 */
	async_event_task_subscribe(EVENT_TASK_STATE_CHANGE, notification_handler,
	    NULL);

	/*
	 * Register at the naming service.
	 */
	errno_t rc = service_register(SERVICE_VFS, INTERFACE_PAGER, vfs_pager, NULL);
	if (rc != EOK) {
		printf("%s: Cannot register VFS pager port: %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_VFS, INTERFACE_VFS, vfs_connection, NULL);
	if (rc != EOK) {
		printf("%s: Cannot register VFS file system port: %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_VFS, INTERFACE_VFS_DRIVER, vfs_connection, NULL);
	if (rc != EOK) {
		printf("%s: Cannot register VFS driver port: %s\n", NAME, str_error(rc));
		return rc;
	}

	/*
	 * Start accepting connections.
	 */
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	return 0;
}

/**
 * @}
 */
