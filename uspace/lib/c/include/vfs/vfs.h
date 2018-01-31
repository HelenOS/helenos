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

#include <stddef.h>
#include <stdint.h>
#include <ipc/vfs.h>
#include <ipc/loc.h>
#include <adt/list.h>
#include <stdio.h>
#include <async.h>
#include <offset.h>

#define MAX_OPEN_FILES	128

enum vfs_change_state_type {
	VFS_PASS_HANDLE
};

typedef enum {
	KIND_FILE,
	KIND_DIRECTORY,
} vfs_file_kind_t;


typedef struct {
	fs_handle_t fs_handle;
	service_id_t service_id;
	fs_index_t index;
	unsigned int lnkcnt;
	bool is_file;
	bool is_directory;
	aoff64_t size;
	service_id_t service;
} vfs_stat_t;

typedef struct {
	char fs_name[FS_NAME_MAXLEN + 1];
	uint32_t f_bsize;    /* fundamental file system block size */
	uint64_t f_blocks;   /* total data blocks in file system */
	uint64_t f_bfree;    /* free blocks in fs */
} vfs_statfs_t;

/** List of file system types */
typedef struct {
	char **fstypes;
	char *buf;
	size_t size;
} vfs_fstypes_t;

extern errno_t vfs_fhandle(FILE *, int *);

extern char *vfs_absolutize(const char *, size_t *);
extern errno_t vfs_clone(int, int, bool, int *);
extern errno_t vfs_cwd_get(char *path, size_t);
extern errno_t vfs_cwd_set(const char *path);
extern async_exch_t *vfs_exchange_begin(void);
extern void vfs_exchange_end(async_exch_t *);
extern errno_t vfs_fsprobe(const char *, service_id_t, vfs_fs_probe_info_t *);
extern errno_t vfs_fstypes(vfs_fstypes_t *);
extern void vfs_fstypes_free(vfs_fstypes_t *);
extern errno_t vfs_link(int, const char *, vfs_file_kind_t, int *);
extern errno_t vfs_link_path(const char *, vfs_file_kind_t, int *);
extern errno_t vfs_lookup(const char *, int, int *);
extern errno_t vfs_lookup_open(const char *, int, int, int *);
extern errno_t vfs_mount_path(const char *, const char *, const char *,
    const char *, unsigned int, unsigned int);
extern errno_t vfs_mount(int, const char *, service_id_t, const char *, unsigned,
    unsigned, int *);
extern errno_t vfs_open(int, int);
extern errno_t vfs_pass_handle(async_exch_t *, int, async_exch_t *);
extern errno_t vfs_put(int);
extern errno_t vfs_read(int, aoff64_t *, void *, size_t, size_t *);
extern errno_t vfs_read_short(int, aoff64_t, void *, size_t, ssize_t *);
extern errno_t vfs_receive_handle(bool, int *);
extern errno_t vfs_rename_path(const char *, const char *);
extern errno_t vfs_resize(int, aoff64_t);
extern int vfs_root(void);
extern errno_t vfs_root_set(int);
extern errno_t vfs_stat(int, vfs_stat_t *);
extern errno_t vfs_stat_path(const char *, vfs_stat_t *);
extern errno_t vfs_statfs(int, vfs_statfs_t *);
extern errno_t vfs_statfs_path(const char *, vfs_statfs_t *);
extern errno_t vfs_sync(int);
extern errno_t vfs_unlink(int, const char *, int);
extern errno_t vfs_unlink_path(const char *);
extern errno_t vfs_unmount(int);
extern errno_t vfs_unmount_path(const char *);
extern errno_t vfs_walk(int, const char *, int, int *);
extern errno_t vfs_write(int, aoff64_t *, const void *, size_t, size_t *);
extern errno_t vfs_write_short(int, aoff64_t, const void *, size_t, ssize_t *);

#endif

/** @}
 */
