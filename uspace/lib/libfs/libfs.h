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

/** @addtogroup libfs 
 * @{
 */ 
/**
 * @file
 */

#ifndef LIBFS_LIBFS_H_
#define	LIBFS_LIBFS_H_ 

#include "../../srv/vfs/vfs.h"
#include <stdint.h>
#include <ipc/ipc.h>
#include <async.h>

typedef struct {
	bool (* match)(void *, const char *);
	void * (* create)(int);
	void (* destroy)(void *);
	bool (* link)(void *, void *, const char *);
	int (* unlink)(void *, void *);
	unsigned long (* index_get)(void *);
	unsigned long (* size_get)(void *);
	unsigned (* lnkcnt_get)(void *);
	void *(* child_get)(void *);
	void *(* sibling_get)(void *);
	void *(* root_get)(void);
	char (* plb_get_char)(unsigned pos);	
	bool (* is_directory)(void *);
	bool (* is_file)(void *);
} libfs_ops_t;

typedef struct {
	int fs_handle;		/**< File system handle. */
	ipcarg_t vfs_phonehash;	/**< Initial VFS phonehash. */
	uint8_t *plb_ro;	/**< Read-only PLB view. */
} fs_reg_t;

extern int fs_register(int, fs_reg_t *, vfs_info_t *, async_client_conn_t);

extern int block_read(int, unsigned long, void *);
extern int block_write(int, unsigned long, void *);

extern void node_add_mp(int, unsigned long);
extern void node_del_mp(int, unsigned long);
extern bool node_is_mp(int, unsigned long);

extern void libfs_lookup(libfs_ops_t *, int, ipc_callid_t, ipc_call_t *);

#endif

/** @}
 */

