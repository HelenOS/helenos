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
 * @file	vfs_file.c
 * @brief	Various operations on files have their home in this file.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <bool.h>
#include <fibril.h>
#include <fibril_sync.h>
#include "vfs.h"

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
 *
 * This resource being per-connection and, in the first place, per-fibril, we
 * don't need to protect it by a mutex.
 */
fibril_local vfs_file_t **files = NULL;

/** Initialize the table of open files. */
bool vfs_files_init(void)
{
	if (!files) {
		files = malloc(MAX_OPEN_FILES * sizeof(vfs_file_t *));
		if (!files)
			return false;
		memset(files, 0, MAX_OPEN_FILES * sizeof(vfs_file_t *));
	}
	return true;
}

/** Allocate a file descriptor.
 *
 * @return		First available file descriptor or a negative error
 *			code.
 */
int vfs_fd_alloc(void)
{
	if (!vfs_files_init())
		return ENOMEM;
	
	unsigned int i;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (!files[i]) {
			files[i] = (vfs_file_t *) malloc(sizeof(vfs_file_t));
			if (!files[i])
				return ENOMEM;
			
			memset(files[i], 0, sizeof(vfs_file_t));
			fibril_mutex_initialize(&files[i]->lock);
			vfs_file_addref(files[i]);
			return (int) i;
		}
	}
	
	return EMFILE;
}

/** Release file descriptor.
 *
 * @param fd		File descriptor being released.
 *
 * @return		EOK on success or EBADF if fd is an invalid file
 *			descriptor.
 */
int vfs_fd_free(int fd)
{
	if (!vfs_files_init())
		return ENOMEM;
	
	if ((fd < 0) || (fd >= MAX_OPEN_FILES) || (files[fd] == NULL))
		return EBADF;
	
	vfs_file_delref(files[fd]);
	files[fd] = NULL;
	
	return EOK;
}

/** Increment reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			incremented.
 */
void vfs_file_addref(vfs_file_t *file)
{
	/*
	 * File structures are per-connection, so no-one, except the current
	 * fibril, should have a reference to them. This is the reason we don't
	 * do any synchronization here.
	 */
	file->refcnt++;
}

/** Decrement reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			decremented.
 */
void vfs_file_delref(vfs_file_t *file)
{
	if (file->refcnt-- == 1) {
		/*
		 * Lost the last reference to a file, need to drop our reference
		 * to the underlying VFS node.
		 */
		vfs_node_delref(file->node);
		free(file);
	}
}

/** Find VFS file structure for a given file descriptor.
 *
 * @param fd		File descriptor.
 *
 * @return		VFS file structure corresponding to fd.
 */
vfs_file_t *vfs_file_get(int fd)
{
	if (!vfs_files_init())
		return NULL;
	
	if ((fd >= 0) && (fd < MAX_OPEN_FILES))
		return files[fd];
	
	return NULL;
}

/**
 * @}
 */
