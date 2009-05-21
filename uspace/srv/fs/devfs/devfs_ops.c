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
 * @file devfs_ops.c
 * @brief Implementation of VFS operations for the devfs file system server.
 */

#include <ipc/ipc.h>
#include <bool.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <libfs.h>
#include "devfs.h"
#include "devfs_ops.h"

#define PLB_GET_CHAR(pos)  (devfs_reg.plb_ro[pos % PLB_SIZE])

bool devfs_init(void)
{
	if (devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING) < 0)
		return false;
	
	return true;
}

void devfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	/* Accept the mount options */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	char *opts = malloc(size + 1);
	if (!opts) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	ipcarg_t retval = ipc_data_write_finalize(callid, opts, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(opts);
		return;
	}
	
	free(opts);
	
	ipc_answer_3(rid, EOK, 0, 0, 0);
}

void devfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void devfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	ipcarg_t first = IPC_GET_ARG1(*request);
	ipcarg_t last = IPC_GET_ARG2(*request);
	dev_handle_t dev_handle = IPC_GET_ARG3(*request);
	ipcarg_t lflag = IPC_GET_ARG4(*request);
	fs_index_t index = IPC_GET_ARG5(*request);
	
	/* Hierarchy is flat, no altroot is supported */
	if (index != 0) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	/* This is a read-only filesystem */
	if ((lflag & L_CREATE) || (lflag & L_LINK) || (lflag & L_UNLINK)) {
		ipc_answer_0(rid, ENOTSUP);
		return;
	}
	
	/* Eat slash */
	if (PLB_GET_CHAR(first) == '/') {
		first++;
		first %= PLB_SIZE;
	}
	
	if (first >= last) {
		/* Root entry */
		if (lflag & L_DIRECTORY)
			ipc_answer_5(rid, EOK, devfs_reg.fs_handle, dev_handle, 0, 0, 0);
		else
			ipc_answer_0(rid, ENOENT);
	} else {
		if (lflag & L_FILE) {
			count_t len;
			if (last >= first)
				len = last - first + 1;
			else
				len = first + PLB_SIZE - last + 1;
			
			char *name = (char *) malloc(len + 1);
			if (name == NULL) {
				ipc_answer_0(rid, ENOMEM);
				return;
			}
			
			count_t i;
			for (i = 0; i < len; i++)
				name[i] = PLB_GET_CHAR(first + i);
			
			name[len] = 0;
			
			dev_handle_t handle;
			if (devmap_device_get_handle(name, &handle, 0) != EOK) {
				free(name);
				ipc_answer_0(rid, ENOENT);
				return;
			}
			
			free(name);
			
			ipc_answer_5(rid, EOK, devfs_reg.fs_handle, dev_handle, handle, 0, 1);
		} else
			ipc_answer_0(rid, ENOENT);
	}
}

void devfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	if (index != 0) {
		ipc_answer_1(rid, ENOENT, 0);
		return;
	}
	
	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_read_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	size_t bytes = 0;
	if (index != 0) {
		(void) ipc_data_read_finalize(callid, NULL, bytes);
	} else {
		count_t count = devmap_device_get_count();
		dev_desc_t *desc = malloc(count * sizeof(dev_desc_t));
		if (desc == NULL) {
			ipc_answer_0(callid, ENOENT);
			ipc_answer_1(rid, ENOENT, 0);
			return;
		}
		
		count_t max = devmap_device_get_devices(count, desc);
		
		if (pos < max) {
			ipc_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
		} else {
			ipc_answer_0(callid, ENOENT);
			ipc_answer_1(rid, ENOENT, 0);
			return;
		}
		
		free(desc);
		bytes = 1;
	}
	
	ipc_answer_1(rid, EOK, bytes);
}

void devfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	/*
	 * Receive the write request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	// TODO
	ipc_answer_0(callid, ENOENT);
	ipc_answer_2(rid, ENOENT, 0, 0);
}

void devfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void devfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

/**
 * @}
 */ 
