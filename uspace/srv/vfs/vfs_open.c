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
 * @file	vfs_open.c
 * @brief	VFS_OPEN method.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <bool.h>
#include <futex.h>
#include <libadt/list.h>
#include <sys/types.h>
#include "vfs.h"

/** Per-connection futex protecting the files array. */
__thread atomic_t files_futex = FUTEX_INITIALIZER;

/**
 * This is a per-connection table of open files.
 * Our assumption is that each client opens only one connection and therefore
 * there is one table of open files per task. However, this may not be the case
 * and the client can open more connections to VFS. In that case, there will be
 * several tables and several file handle name spaces per task. Besides of this,
 * the functionality will stay unchanged. So unless the client knows what it is
 * doing, it should open one connection to VFS only.
 *
 * Allocation of the open files table is deferred until the client makes the
 * first VFS_OPEN operation.
 */
__thread vfs_file_t *files = NULL;

/** Initialize the table of open files. */
static bool vfs_conn_open_files_init(void)
{
	/*
	 * Optimized fast path that will never go to sleep unnecessarily.
	 * The assumption is that once files is non-zero, it will never be zero
	 * again.
	 */
	if (files)
		return true;
		
	futex_down(&files_futex);
	if (!files) {
		files = malloc(MAX_OPEN_FILES * sizeof(vfs_file_t));
		if (!files) {
			futex_up(&files_futex);
			return false;
		}
		memset(files, 0, MAX_OPEN_FILES * sizeof(vfs_file_t));
	}
	futex_up(&files_futex);
	return true;
}

void vfs_open(ipc_callid_t rid, ipc_call_t *request)
{
	if (!vfs_conn_open_files_init()) {
		ipc_answer_fast_0(rid, ENOMEM);
		return;
	}

	/*
	 * The POSIX interface is open(path, flags, mode).
	 * We can receive flags and mode along with the VFS_OPEN call; the path
	 * will need to arrive in another call.
	 */
	int flags = IPC_GET_ARG1(*request);
	int mode = IPC_GET_ARG2(*request);
	size_t size;

	ipc_callid_t callid;
	ipc_call_t call;

	if (!ipc_data_receive(&callid, &call, NULL, &size)) {
		ipc_answer_fast_0(callid, EINVAL);
		ipc_answer_fast_0(rid, EINVAL);
		return;
	}

	/*
	 * Now we are on the verge of accepting the path.
	 *
	 * There is one optimization we could do in the future: copy the path
	 * directly into the PLB using some kind of a callback.
	 */
	char *path = malloc(size);
	
	if (!path) {
		ipc_answer_fast_0(callid, ENOMEM);
		ipc_answer_fast_0(rid, ENOMEM);
		return;
	}

	int rc;
	if (rc = ipc_data_deliver(callid, &call, path, size)) {
		ipc_answer_fast_0(rid, rc);
		free(path);
		return;
	}
	
	/*
	 * The path is now populated and we can call vfs_lookup_internal().
	 */
	vfs_triplet_t triplet;
	rc = vfs_lookup_internal(path, size, &triplet, NULL);
	if (rc) {
		ipc_answer_fast_0(rid, rc);
		free(path);
		return;
	}

	/*
	 * Path is no longer needed.
	 */
	free(path);

	vfs_node_t *node = vfs_node_get(&triplet);
	// TODO: not finished	
}

/**
 * @}
 */ 
