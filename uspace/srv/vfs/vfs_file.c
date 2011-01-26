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
#include <str.h>
#include <assert.h>
#include <bool.h>
#include <fibril.h>
#include <fibril_synch.h>
#include "vfs.h"

#define VFS_DATA	((vfs_client_data_t *) async_client_data_get())
#define FILES		(VFS_DATA->files)

typedef struct {
	fibril_mutex_t lock;
	vfs_file_t **files;
} vfs_client_data_t;

/** Initialize the table of open files. */
static bool vfs_files_init(void)
{
	fibril_mutex_lock(&VFS_DATA->lock);
	if (!FILES) {
		FILES = malloc(MAX_OPEN_FILES * sizeof(vfs_file_t *));
		if (!FILES) {
			fibril_mutex_unlock(&VFS_DATA->lock);
			return false;
		}
		memset(FILES, 0, MAX_OPEN_FILES * sizeof(vfs_file_t *));
	}
	fibril_mutex_unlock(&VFS_DATA->lock);
	return true;
}

/** Cleanup the table of open files. */
static void vfs_files_done(void)
{
	int i;

	if (!FILES)
		return;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (FILES[i]) {
			(void) vfs_close_internal(FILES[i]);
			(void) vfs_fd_free(i);
		}
	}
	
	free(FILES);
}

void *vfs_client_data_create(void)
{
	vfs_client_data_t *vfs_data;

	vfs_data = malloc(sizeof(vfs_client_data_t));
	if (vfs_data) {
		fibril_mutex_initialize(&vfs_data->lock);
		vfs_data->files = NULL;
	}
	
	return vfs_data;
}

void vfs_client_data_destroy(void *data)
{
	vfs_client_data_t *vfs_data = (vfs_client_data_t *) data;

	vfs_files_done();
	free(vfs_data);
}

/** Increment reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			incremented.
 */
static void vfs_file_addref(vfs_file_t *file)
{
	assert(fibril_mutex_is_locked(&VFS_DATA->lock));

	file->refcnt++;
}

/** Decrement reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			decremented.
 */
static void vfs_file_delref(vfs_file_t *file)
{
	assert(fibril_mutex_is_locked(&VFS_DATA->lock));

	if (file->refcnt-- == 1) {
		/*
		 * Lost the last reference to a file, need to drop our reference
		 * to the underlying VFS node.
		 */
		vfs_node_delref(file->node);
		free(file);
	}
}


/** Allocate a file descriptor.
 *
 * @param desc If true, look for an available file descriptor
 *             in a descending order.
 *
 * @return First available file descriptor or a negative error
 *         code.
 */
int vfs_fd_alloc(bool desc)
{
	if (!vfs_files_init())
		return ENOMEM;
	
	unsigned int i;
	if (desc)
		i = MAX_OPEN_FILES - 1;
	else
		i = 0;
	
	fibril_mutex_lock(&VFS_DATA->lock);
	while (true) {
		if (!FILES[i]) {
			FILES[i] = (vfs_file_t *) malloc(sizeof(vfs_file_t));
			if (!FILES[i]) {
				fibril_mutex_unlock(&VFS_DATA->lock);
				return ENOMEM;
			}
			
			memset(FILES[i], 0, sizeof(vfs_file_t));
			fibril_mutex_initialize(&FILES[i]->lock);
			vfs_file_addref(FILES[i]);
			fibril_mutex_unlock(&VFS_DATA->lock);
			return (int) i;
		}
		
		if (desc) {
			if (i == 0)
				break;
			
			i--;
		} else {
			if (i == MAX_OPEN_FILES - 1)
				break;
			
			i++;
		}
	}
	fibril_mutex_unlock(&VFS_DATA->lock);
	
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

	fibril_mutex_lock(&VFS_DATA->lock);	
	if ((fd < 0) || (fd >= MAX_OPEN_FILES) || (FILES[fd] == NULL)) {
		fibril_mutex_unlock(&VFS_DATA->lock);
		return EBADF;
	}
	
	vfs_file_delref(FILES[fd]);
	FILES[fd] = NULL;
	fibril_mutex_unlock(&VFS_DATA->lock);
	
	return EOK;
}

/** Assign a file to a file descriptor.
 *
 * @param file File to assign.
 * @param fd   File descriptor to assign to.
 *
 * @return EOK on success or EINVAL if fd is an invalid or already
 *         used file descriptor.
 *
 */
int vfs_fd_assign(vfs_file_t *file, int fd)
{
	if (!vfs_files_init())
		return ENOMEM;

	fibril_mutex_lock(&VFS_DATA->lock);	
	if ((fd < 0) || (fd >= MAX_OPEN_FILES) || (FILES[fd] != NULL)) {
		fibril_mutex_unlock(&VFS_DATA->lock);
		return EINVAL;
	}
	
	FILES[fd] = file;
	vfs_file_addref(FILES[fd]);
	fibril_mutex_unlock(&VFS_DATA->lock);
	
	return EOK;
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
	
	fibril_mutex_lock(&VFS_DATA->lock);
	if ((fd >= 0) && (fd < MAX_OPEN_FILES)) {
		vfs_file_t *file = FILES[fd];
		fibril_mutex_unlock(&VFS_DATA->lock);
		return file;
	}
	fibril_mutex_unlock(&VFS_DATA->lock);
	
	return NULL;
}

/**
 * @}
 */
