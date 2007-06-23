/*
 * Copyright (c) 2007 Michal Konopa
 * Copyright (c) 2007 Martin Jelen
 * Copyright (c) 2007 Peter Majer
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

/** @addtogroup libc
 * @{
 */ 

/**
 * @file	file.c
 * @brief	The user library for working with the file system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <async.h>
#include <ipc/services.h>
#include <ipc/ipc.h>
#include <sys/mman.h>
#include <io/file.h>
#include <bool.h>
#include <err.h>
#include <align.h>
#include "../../../fs/dir.h"
#include "../../../share/message.h"
#include "../../../share/shared_proto.h"

#define CONNECT_SLEEP_INTERVAL 10000
#define CONNECT_SLEEP_TIMEOUT 100000

/**
*
*/
static int fs_phone;

static file_t *file_connect();
static int file_disconnect(file_t *file);

/**
 * Connect to the FS task and share memory with it for further data and 
 * extended memory transfers
 */
file_t *file_connect() {
	file_t *result;
	
	size_t size;
	void *share = NULL;
	int retval;

	size = ALIGN_UP(BLOCK_SIZE * sizeof(char), PAGE_SIZE);
	share = mmap(share, size, AS_AREA_READ | AS_AREA_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if ((int) share < 0) {
		f_err = F_MMAP_FAILURE;
		return NULL;
	}

	retval = async_req_2(fs_phone, FS_NEW_CONSUMER, task_get_id(), 0, NULL, NULL);
	if (retval < 0) {
		f_err = F_COMM_FAILURE;
		return NULL;
	}

	int flags = 0;
	flags = AS_AREA_READ | AS_AREA_WRITE;
	retval = async_req_3(fs_phone, IPC_M_AS_AREA_SEND, (uintptr_t)share, size, flags, NULL, NULL, NULL);
	if (retval < 0) {
		f_err = F_COMM_FAILURE;
		return NULL;
	}
	
	/* Allocating structure for extended message. */
	message_params_t *params = malloc(sizeof(message_params_t));
	memset((void*) params, 0, sizeof(message_params_t));
	
	result = malloc(sizeof(file_t));
	result->share = share;
	result->size = size;
	result->params = params;
	
	f_err = F_OK;
	return result;
}

/**
 * Disconnect from the FS task, unsharing memory and freeing the file data structure
 */
int file_disconnect(file_t *file) {
	int retval = send_request(fs_phone, FS_DROP_CONSUMER, file->params, file->share);
	if (retval < 0)
		return -1;
	
	/* Unmapping share area. */ 
	retval = munmap(file->share, file->size);
	if (retval < 0)
		return -1;
	
	file->size = ALIGN_UP(BLOCK_SIZE * sizeof(char), PAGE_SIZE);
	file->share = mmap(file->share, file->size, AS_AREA_READ | AS_AREA_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if ((int) (file->share) < 0)
		return -1;
	
	free(file);
	f_err = F_OK;
	return F_OK;
}

/**
 * List contents of the current directory
 */
dir_item_t *ls(unsigned int *length) {
	dir_item_t *result;
	unsigned short entries_num;

	file_t *shared_file = file_connect();
	if (shared_file == NULL)
		return NULL;
	
	/* We want lookup our work directory. */
	retval = send_request(fs_phone, FS_DSUM, shared_file->params, shared_file->share);
	if (retval < 0) {
		f_err = F_READ_ERROR;
		return NULL;
	}
	
	entries_num = retval;
	*length = entries_num;
	
	result = malloc(entries_num * sizeof (dir_item_t));
	
	int entry;
	for (entry = 0; entry < entries_num; entry++) {
		shared_file->params->entry_number = entry;
		retval = send_request(fs_phone, FS_READENTRY, shared_file->params, shared_file->share);
		if (retval < 0) {
			f_err = F_READ_ERROR;
			return NULL;
		}
		
		memcpy(&(result[entry].inode_num), shared_file->share, sizeof(unsigned short));
		memcpy(result[entry].name, (void *)(shared_file->share+sizeof(unsigned short)), retval-sizeof(unsigned short));

		/* Do not show empty entries. */
		if (!result[entry].inode_num)
			continue;
		 
	}
	return result;
}

/**
 * Change the current working directory for the task
 */
int chdir(char * new_dir)
{
	file_t *shared_file = file_connect();
	
	if (shared_file == NULL) {
		f_err = F_READ_ERROR;
		return F_READ_ERROR;
	}
	memcpy(shared_file->params->fname, new_dir, 30);
	
	int retval = send_request(fs_phone, FS_CHDIR, shared_file->params, shared_file->share);
	if (retval < 0) {
		f_err = F_READ_ERROR;
		return F_READ_ERROR;
	}

	retval = file_disconnect(shared_file);
	f_err = F_OK;
	return F_OK;
}

/**
 * Open a file for reading and/or writing
 */
file_t *fopen(char *name, int mode)
{
	file_t *file = file_connect();
	
	/* We want to work with the specified file. */	
	memcpy(file->params->fname, name, 30);

	int retval = send_request(fs_phone, FS_OPEN, file->params, file->share);
	if (retval < 0)
		return NULL;
	
	file->handle = retval;
	
	return file;
}

/**
 * Read status information about a file
 */
int fstat(file_t *file)
{
	memcpy(file->params->fname, file->base_info.name, 30);
	file->params->fd = file->handle;

	int retval = send_request(fs_phone, FS_FSTAT, file->params, file->share);
	if (retval < 0)
		return -1;
	
	memcpy((void *)(&file->stat), file->share, sizeof(stat_t));
	
	f_err = F_OK;
	return F_OK;
}

/**
 * Read data from a file
 */
int fread(file_t *file, void* buffer, unsigned int size)
{
	file->params->nbytes = size;
	
	int retval = send_request(fs_phone, FS_READ, file->params, file->share);
	if (retval < 0)
		return -1;

	f_err = F_OK;
	return F_OK;
}

/**
 * Seek to a position within a file
 */
int fseek(file_t *file, int offset, int whence)
{
	file->params->offset = 0;
	file->params->whence = 0;	/* from beginning of the file */

	int retval = send_request(fs_phone, FS_SEEK, file->params, file->share);
	if (retval < 0)
		return -1;

	f_err = F_OK;
	return F_OK;
}

/**
 * Close a file
 */
int fclose(file_t *file)
{
	int retval = send_request(fs_phone, FS_CLOSE, file->params, file->share);
	if (retval < 0)
		return -1;

	if (file != NULL)
		file_disconnect(file);
	
	f_err = F_OK;
	return F_OK;
}

/**
 *@}
 */
