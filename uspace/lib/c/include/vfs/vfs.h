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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_VFS_H_
#define LIBC_VFS_H_

#include <sys/types.h>
#include <ipc/vfs.h>
#include <ipc/loc.h>
#include <adt/list.h>
#include <stdio.h>
#include <async.h>

#define MAX_OPEN_FILES	128

enum vfs_change_state_type {
	VFS_PASS_HANDLE
};

struct stat {
	fs_handle_t fs_handle;
	service_id_t service_id;
	fs_index_t index;
	unsigned int lnkcnt;
	bool is_file;
	bool is_directory;
	aoff64_t size;
	service_id_t service;
};

struct statfs { 
	char fs_name[FS_NAME_MAXLEN + 1];
	uint32_t f_bsize;    /* fundamental file system block size */
	uint64_t f_blocks;   /* total data blocks in file system */
	uint64_t f_bfree;    /* free blocks in fs */
};

extern char *vfs_absolutize(const char *, size_t *);

extern int mount(const char *, const char *, const char *, const char *,
    unsigned int, unsigned int);
extern int unmount(const char *);

extern int vfs_fhandle(FILE *, int *);

extern int vfs_get_mtab_list(list_t *mtab_list);

extern async_exch_t *vfs_exchange_begin(void);
extern void vfs_exchange_end(async_exch_t *);

extern int _vfs_walk(int, const char *, int);
extern int _vfs_open(int, int);
extern int vfs_lookup(const char *, int);

extern int vfs_pass_handle(async_exch_t *, int, async_exch_t *);
extern int vfs_receive_handle(bool);

extern int vfs_clone(int, int, bool);
extern int vfs_root(void);
extern void vfs_root_set(int);
extern int vfs_resize(int, aoff64_t);
extern int vfs_stat(int, struct stat *);
extern int vfs_stat_path(const char *, struct stat *);
extern int vfs_statfs(int, struct statfs *);
extern int vfs_statfs_path(const char *, struct statfs *);
extern int vfs_unlink(int, const char *, int);
extern int vfs_unlink_path(const char *);

int vfs_mount(int, const char *, service_id_t, const char *, unsigned, unsigned, int *);
int vfs_unmount(int);

#endif

/** @}
 */
