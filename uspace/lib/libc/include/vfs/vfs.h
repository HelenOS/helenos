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
#include <ipc/devmap.h>
#include <stdio.h>

/**
 * This type is a libc version of the VFS triplet.
 * It uniquelly identifies a file system node within a file system instance.
 */
typedef struct {
	fs_handle_t fs_handle;
	dev_handle_t dev_handle;
	fs_index_t index;
} fdi_node_t;

extern char *absolutize(const char *, size_t *);

extern int mount(const char *, const char *, const char *, const char *,
    unsigned int);

extern void __stdio_init(int filc, fdi_node_t *filv[]);
extern void __stdio_done(void);

extern int open_node(fdi_node_t *, int);
extern int fd_phone(int);
extern int fd_node(int, fdi_node_t *);

extern FILE *fopen_node(fdi_node_t *, const char *);
extern int fphone(FILE *);
extern int fnode(FILE *, fdi_node_t *);

#endif

/** @}
 */
