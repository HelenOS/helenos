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
#include <rwlock.h>
#include <sys/types.h>
#include <stdlib.h>
#include "vfs.h"

void vfs_open(ipc_callid_t rid, ipc_call_t *request)
{
	if (!vfs_files_init()) {
		ipc_answer_0(rid, ENOMEM);
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

	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
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
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	int rc;
	if ((rc = ipc_data_write_finalize(callid, path, size))) {
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}
	
	/*
	 * Avoid the race condition in which the file can be deleted before we
	 * find/create-and-lock the VFS node corresponding to the looked-up
	 * triplet.
	 */
	rwlock_reader_lock(&namespace_rwlock);

	/*
	 * The path is now populated and we can call vfs_lookup_internal().
	 */
	vfs_triplet_t triplet;
	rc = vfs_lookup_internal(path, size, &triplet, NULL);
	if (rc) {
		rwlock_reader_unlock(&namespace_rwlock);
		ipc_answer_0(rid, rc);
		free(path);
		return;
	}

	/*
	 * Path is no longer needed.
	 */
	free(path);

	vfs_node_t *node = vfs_node_get(&triplet);
	rwlock_reader_unlock(&namespace_rwlock);

	/*
	 * Get ourselves a file descriptor and the corresponding vfs_file_t
	 * structure.
	 */
	int fd = vfs_fd_alloc();
	if (fd < 0) {
		vfs_node_put(node);
		ipc_answer_0(rid, fd);
		return;
	}
	vfs_file_t *file = vfs_file_get(fd);
	file->node = node;

	/*
	 * The following increase in reference count is for the fact that the
	 * file is being opened and that a file structure is pointing to it.
	 * It is necessary so that the file will not disappear when
	 * vfs_node_put() is called. The reference will be dropped by the
	 * respective VFS_CLOSE.
	 */
	vfs_node_addref(node);
	vfs_node_put(node);

	/*
	 * Success! Return the new file descriptor to the client.
	 */
	ipc_answer_1(rid, EOK, fd);
}

/**
 * @}
 */ 
