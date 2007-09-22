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
 * @brief	VFS_REGISTER method.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <bool.h>
#include <futex.h>
#include <libadt/list.h>
#include "vfs.h"

atomic_t fs_head_futex = FUTEX_INITIALIZER;
link_t fs_head;

/** Verify the VFS info structure.
 *
 * @param info		Info structure to be verified.
 *
 * @return		Non-zero if the info structure is sane, zero otherwise.
 */
static bool vfs_info_sane(vfs_info_t *info)
{
	int i;

	/*
	 * Check if the name is non-empty and is composed solely of ASCII
	 * characters [a-z]+[a-z0-9_-]*.
	 */
	if (!islower(info->name[0])) {
		dprintf("The name doesn't start with a lowercase character.\n");
		return false;
	}
	for (i = 1; i < FS_NAME_MAXLEN; i++) {
		if (!(islower(info->name[i]) || isdigit(info->name[i])) &&
		    (info->name[i] != '-') && (info->name[i] != '_')) {
			if (info->name[i] == '\0') {
				break;
			} else {
				dprintf("The name contains illegal "
				    "characters.\n");
				return false;
			}
		}
	}
	

	/*
	 * Check if the FS implements mandatory VFS operations.
	 */
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_REGISTER)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_REGISTER not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_MOUNT)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_MOUNT not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_UNMOUNT)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_UNMOUNT not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_LOOKUP)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_LOOKUP not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_OPEN)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_OPEN not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_CLOSE)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_CLOSE not defined by the client.\n");
		return false;
	}
	if (info->ops[IPC_METHOD_TO_VFS_OP(VFS_READ)] != VFS_OP_DEFINED) {
		dprintf("Operation VFS_READ not defined by the client.\n");
		return false;
	}
	
	/*
	 * Check if each operation is either not defined, defined or default.
	 */
	for (i = VFS_FIRST; i < VFS_LAST; i++) {
		if ((info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_NULL) && 
		    (info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_DEFAULT) && 
		    (info->ops[IPC_METHOD_TO_VFS_OP(i)] != VFS_OP_DEFINED)) {
			dprintf("Operation info not understood.\n");
			return false;
		}
	}
	return true;
}

/** VFS_REGISTER protocol function.
 *
 * @param rid		Hash of the call with the request.
 * @param request	Call structure with the request.
 */
void vfs_register(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int rc;
	size_t size;

	dprintf("Processing VFS_REGISTER request received from %p.\n",
		request->in_phone_hash);

	/*
	 * The first call has to be IPC_M_DATA_SEND in which we receive the
	 * VFS info structure from the client FS.
	 */
	if (!ipc_data_receive(&callid, &call, NULL, &size)) {
		/*
		 * The client doesn't obey the same protocol as we do.
		 */
		dprintf("Receiving of VFS info failed.\n");
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
	
	dprintf("VFS info received, size = %d\n", size);
	
	/*
	 * We know the size of the VFS info structure. See if the client
	 * understands this easy concept too.
	 */
	if (size != sizeof(vfs_info_t)) {
		/*
		 * The client is sending us something, which cannot be
		 * the info structure.
		 */
		dprintf("Received VFS info has bad size.\n");
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
	fs_info_t *fs_info;

	/*
	 * Allocate and initialize a buffer for the fs_info structure.
	 */
	fs_info = (fs_info_t *) malloc(sizeof(fs_info_t));
	if (!fs_info) {
		dprintf("Could not allocate memory for FS info.\n");
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		ipc_answer_fast(rid, ENOMEM, 0, 0);
		return;
	}
	link_initialize(&fs_info->fs_link);
		
	rc = ipc_data_deliver(callid, &call, &fs_info->vfs_info, size);
	if (rc != EOK) {
		dprintf("Failed to deliver the VFS info into our AS, rc=%d.\n",
		    rc);
		free(fs_info);
		ipc_answer_fast(callid, rc, 0, 0);
		ipc_answer_fast(rid, rc, 0, 0);
		return;
	}

	dprintf("VFS info delivered.\n");
		
	if (!vfs_info_sane(&fs_info->vfs_info)) {
		free(fs_info);
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
		
	futex_down(&fs_head_futex);

	/*
	 * Check for duplicit registrations.
	 */
	link_t *cur;
	for (cur = fs_head.next; cur != &fs_head; cur = cur->next) {
		fs_info_t *fi = list_get_instance(cur, fs_info_t,
		    fs_link);
		/* TODO: replace strcmp with strncmp once we have it */
		if (strcmp(fs_info->vfs_info.name, fi->vfs_info.name) == 0) {
			/*
			 * We already register a fs like this.
			 */
			dprintf("FS is already registered.\n");
			futex_up(&fs_head_futex);
			free(fs_info);
			ipc_answer_fast(callid, EEXISTS, 0, 0);
			ipc_answer_fast(rid, EEXISTS, 0, 0);
			return;
		}
	}

	/*
	 * Add fs_info to the list of registered FS's.
	 */
	dprintf("Adding FS into the registered list.\n");
	list_append(&fs_info->fs_link, &fs_head);

	/*
	 * ACK receiving a properly formatted, non-duplicit vfs_info.
	 */
	ipc_answer_fast(callid, EOK, 0, 0);
	
	/*
	 * Now we want the client to send us the IPC_M_CONNECT_TO_ME call so
	 * that a callback connection is created and we have a phone through
	 * which to forward VFS requests to it.
	 */
	callid = async_get_call(&call);
	if (IPC_GET_METHOD(call) != IPC_M_CONNECT_TO_ME) {
		dprintf("Unexpected call, method = %d\n", IPC_GET_METHOD(call));
		list_remove(&fs_info->fs_link);
		futex_up(&fs_head_futex);
		free(fs_info);
		ipc_answer_fast(callid, EINVAL, 0, 0);
		ipc_answer_fast(rid, EINVAL, 0, 0);
		return;
	}
	fs_info->phone = IPC_GET_ARG3(call);
	ipc_answer_fast(callid, EOK, 0, 0);

	dprintf("Callback connection to FS created.\n");

	futex_up(&fs_head_futex);

	/*
	 * That was it. The FS has been registered.
	 */
	ipc_answer_fast(rid, EOK, 0, 0);
	dprintf("\"%s\" filesystem successfully registered.\n",
	    fs_info->vfs_info.name);

}

/**
 * @}
 */ 
