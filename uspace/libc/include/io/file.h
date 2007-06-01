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
 * @file	file.h
 * @brief	The main header for the user library for working with the file system
 */

#ifndef _FILE_H
#define _FILE_H

#include "../../../fs/const.h"
#include "../../../fs/type.h"
#include "../../../fs/stat.h"
#include "../../../fs/dir.h"
#include "../../../share/message.h"

#define F_OK			0x00
#define F_FILE_NOT_FOUND	0x01
#define F_FILE_NOT_OPEN		0x02
#define F_READ_ERROR		0x10
#define F_READ_OVERFLOW		0x11
#define F_SYSTEM_ERROR		0xf0
#define F_IPC_FAILURE		0xf1
#define F_MMAP_FAILURE		0xf2
#define F_COMM_FAILURE		0xf3

#define F_ERRTYPE_MASK		0xf0

#define F_MODE_READ		0x01
#define F_MODE_WRITE		0x02
#define F_MODE_READ_WRITE	F_MODE_READ | F_MODE_WRITE
#define F_MODE_APPEND		0x04

/**
 *
 */
typedef struct {
	char name[30];
	unsigned short inode_num;
} dir_item_t;

/**
 *
 */
typedef struct {
	size_t size;
	dir_item_t base_info;
	void *share;
	message_params_t *params;
	unsigned int handle;
	stat_t stat;
} file_t;

static int f_err;

dir_item_t *ls(unsigned int *length);
int chdir(char *new_dir);

file_t *fopen(char *name, int mode);
int fstat(file_t *file);
int fread(file_t *file, void *buffer, unsigned int size);
int fseek(file_t * file, int offset, int whence);
int fclose(file_t *file);

#endif

/**
 *@}
 /
