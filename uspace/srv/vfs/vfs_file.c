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
#include <stdbool.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <task.h>
#include "vfs.h"

#define VFS_DATA	((vfs_client_data_t *) async_get_client_data())
#define FILES		(VFS_DATA->files)

typedef struct {
	fibril_mutex_t lock;
	fibril_condvar_t cv;
	list_t passed_handles;
	vfs_file_t **files;
} vfs_client_data_t;

typedef struct {
	link_t link;
	int handle;
} vfs_boxed_handle_t;

static int _vfs_fd_free(vfs_client_data_t *, int);

/** Initialize the table of open files. */
static bool vfs_files_init(vfs_client_data_t *vfs_data)
{
	fibril_mutex_lock(&vfs_data->lock);
	if (!vfs_data->files) {
		vfs_data->files = malloc(MAX_OPEN_FILES * sizeof(vfs_file_t *));
		if (!vfs_data->files) {
			fibril_mutex_unlock(&vfs_data->lock);
			return false;
		}
		memset(vfs_data->files, 0, MAX_OPEN_FILES * sizeof(vfs_file_t *));
	}
	fibril_mutex_unlock(&vfs_data->lock);
	return true;
}

/** Cleanup the table of open files. */
static void vfs_files_done(vfs_client_data_t *vfs_data)
{
	int i;

	if (!vfs_data->files)
		return;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (vfs_data->files[i])
			(void) _vfs_fd_free(vfs_data, i);
	}
	
	free(vfs_data->files);

	while (!list_empty(&vfs_data->passed_handles)) {
		link_t *lnk;
		vfs_boxed_handle_t *bh;
		
		lnk = list_first(&vfs_data->passed_handles);
		list_remove(lnk);

		bh = list_get_instance(lnk, vfs_boxed_handle_t, link);
		free(bh);
	}
}

void *vfs_client_data_create(void)
{
	vfs_client_data_t *vfs_data;

	vfs_data = malloc(sizeof(vfs_client_data_t));
	if (vfs_data) {
		fibril_mutex_initialize(&vfs_data->lock);
		fibril_condvar_initialize(&vfs_data->cv);
		list_initialize(&vfs_data->passed_handles);
		vfs_data->files = NULL;
	}
	
	return vfs_data;
}

void vfs_client_data_destroy(void *data)
{
	vfs_client_data_t *vfs_data = (vfs_client_data_t *) data;

	vfs_files_done(vfs_data);
	free(vfs_data);
}

/** Close the file in the endpoint FS server. */
static int vfs_file_close_remote(vfs_file_t *file)
{
	assert(!file->refcnt);
	
	async_exch_t *exch = vfs_exchange_grab(file->node->fs_handle);
	
	ipc_call_t answer;
	aid_t msg = async_send_2(exch, VFS_OUT_CLOSE, file->node->service_id,
	    file->node->index, &answer);
	
	vfs_exchange_release(exch);
	
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	return IPC_GET_ARG1(answer);
}

/** Increment reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			incremented.
 */
static void vfs_file_addref(vfs_client_data_t *vfs_data, vfs_file_t *file)
{
	assert(fibril_mutex_is_locked(&vfs_data->lock));

	file->refcnt++;
}

/** Decrement reference count of VFS file structure.
 *
 * @param file		File structure that will have reference count
 *			decremented.
 */
static int vfs_file_delref(vfs_client_data_t *vfs_data, vfs_file_t *file)
{
	int rc = EOK;

	assert(fibril_mutex_is_locked(&vfs_data->lock));

	if (file->refcnt-- == 1) {
		/*
		 * Lost the last reference to a file, need to close it in the
		 * endpoint FS and drop our reference to the underlying VFS node.
		 */
		rc = vfs_file_close_remote(file);
		vfs_node_delref(file->node);
		free(file);
	}

	return rc;
}

static int _vfs_fd_alloc(vfs_client_data_t *vfs_data, bool desc)
{
	if (!vfs_files_init(vfs_data))
		return ENOMEM;
	
	unsigned int i;
	if (desc)
		i = MAX_OPEN_FILES - 1;
	else
		i = 0;
	
	fibril_mutex_lock(&vfs_data->lock);
	while (true) {
		if (!vfs_data->files[i]) {
			vfs_data->files[i] = (vfs_file_t *) malloc(sizeof(vfs_file_t));
			if (!vfs_data->files[i]) {
				fibril_mutex_unlock(&vfs_data->lock);
				return ENOMEM;
			}
			
			memset(vfs_data->files[i], 0, sizeof(vfs_file_t));
			fibril_mutex_initialize(&vfs_data->files[i]->lock);
			vfs_file_addref(vfs_data, vfs_data->files[i]);
			fibril_mutex_unlock(&vfs_data->lock);
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
	fibril_mutex_unlock(&vfs_data->lock);
	
	return EMFILE;
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
	return _vfs_fd_alloc(VFS_DATA, desc);
}

static int _vfs_fd_free(vfs_client_data_t *vfs_data, int fd)
{
	int rc;

	if (!vfs_files_init(vfs_data))
		return ENOMEM;

	fibril_mutex_lock(&vfs_data->lock);	
	if ((fd < 0) || (fd >= MAX_OPEN_FILES) || !vfs_data->files[fd]) {
		fibril_mutex_unlock(&vfs_data->lock);
		return EBADF;
	}
	
	rc = vfs_file_delref(vfs_data, vfs_data->files[fd]);
	vfs_data->files[fd] = NULL;
	fibril_mutex_unlock(&vfs_data->lock);
	
	return rc;
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
	return _vfs_fd_free(VFS_DATA, fd);
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
	if (!vfs_files_init(VFS_DATA))
		return ENOMEM;

	fibril_mutex_lock(&VFS_DATA->lock);	
	if ((fd < 0) || (fd >= MAX_OPEN_FILES) || (FILES[fd] != NULL)) {
		fibril_mutex_unlock(&VFS_DATA->lock);
		return EINVAL;
	}
	
	FILES[fd] = file;
	vfs_file_addref(VFS_DATA, FILES[fd]);
	fibril_mutex_unlock(&VFS_DATA->lock);
	
	return EOK;
}

static vfs_file_t *_vfs_file_get(vfs_client_data_t *vfs_data, int fd)
{
	if (!vfs_files_init(vfs_data))
		return NULL;
	
	fibril_mutex_lock(&vfs_data->lock);
	if ((fd >= 0) && (fd < MAX_OPEN_FILES)) {
		vfs_file_t *file = vfs_data->files[fd];
		if (file != NULL) {
			vfs_file_addref(vfs_data, file);
			fibril_mutex_unlock(&vfs_data->lock);
			return file;
		}
	}
	fibril_mutex_unlock(&vfs_data->lock);
	
	return NULL;
}

/** Find VFS file structure for a given file descriptor.
 *
 * @param fd		File descriptor.
 *
 * @return		VFS file structure corresponding to fd.
 */
vfs_file_t *vfs_file_get(int fd)
{
	return _vfs_file_get(VFS_DATA, fd);
}

static void _vfs_file_put(vfs_client_data_t *vfs_data, vfs_file_t *file)
{
	fibril_mutex_lock(&vfs_data->lock);
	vfs_file_delref(vfs_data, file);
	fibril_mutex_unlock(&vfs_data->lock);
}

/** Stop using a file structure.
 *
 * @param file		VFS file structure.
 */
void vfs_file_put(vfs_file_t *file)
{
	_vfs_file_put(VFS_DATA, file);
}

void vfs_pass_handle(task_id_t donor_id, task_id_t acceptor_id, int donor_fd)
{
	vfs_client_data_t *donor_data = NULL;
	vfs_client_data_t *acceptor_data = NULL;
	vfs_file_t *donor_file = NULL;
	vfs_file_t *acceptor_file = NULL;
	vfs_boxed_handle_t *bh;
	int acceptor_fd;

	acceptor_data = async_get_client_data_by_id(acceptor_id);
	if (!acceptor_data)
		return;

	bh = malloc(sizeof(vfs_boxed_handle_t));
	assert(bh);

	link_initialize(&bh->link);
	bh->handle = -1;

	donor_data = async_get_client_data_by_id(donor_id);
	if (!donor_data)
		goto out;

	donor_file = _vfs_file_get(donor_data, donor_fd);
	if (!donor_file)
		goto out;

	acceptor_fd = _vfs_fd_alloc(acceptor_data, false);
	if (acceptor_fd < 0)
		goto out;

	bh->handle = acceptor_fd;

	/*
	 * Add a new reference to the underlying VFS node.
	 */
	vfs_node_addref(donor_file->node);
	(void) vfs_open_node_remote(donor_file->node);

	acceptor_file = _vfs_file_get(acceptor_data, acceptor_fd);
	assert(acceptor_file);

	/*
	 * Inherit attributes from the donor.
	 */
	acceptor_file->node = donor_file->node;
	acceptor_file->append = donor_file->append;
	acceptor_file->pos = donor_file->pos;

out:
	fibril_mutex_lock(&acceptor_data->lock);
	list_append(&bh->link, &acceptor_data->passed_handles);
	fibril_condvar_broadcast(&acceptor_data->cv);
	fibril_mutex_unlock(&acceptor_data->lock);

	if (donor_data)
		async_put_client_data_by_id(donor_id);
	if (acceptor_data)
		async_put_client_data_by_id(acceptor_id);
	if (donor_file)
		_vfs_file_put(donor_data, donor_file);
	if (acceptor_file)
		_vfs_file_put(acceptor_data, acceptor_file);

}

int vfs_wait_handle_internal(void)
{
	vfs_client_data_t *vfs_data = VFS_DATA;	
	int fd;
	
	fibril_mutex_lock(&vfs_data->lock);
	while (list_empty(&vfs_data->passed_handles))
		fibril_condvar_wait(&vfs_data->cv, &vfs_data->lock);
	link_t *lnk = list_first(&vfs_data->passed_handles);
	list_remove(lnk);
	fibril_mutex_unlock(&vfs_data->lock);

	vfs_boxed_handle_t *bh = list_get_instance(lnk, vfs_boxed_handle_t, link);
	fd = bh->handle;
	free(bh);

	return fd;
}

/**
 * @}
 */
