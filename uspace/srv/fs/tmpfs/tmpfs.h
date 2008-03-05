/*
 * Copyright (c) 2008 Jakub Jermar
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

#ifndef TMPFS_TMPFS_H_
#define TMPFS_TMPFS_H_

#include <ipc/ipc.h>
#include <libfs.h>
#include <atomic.h>
#include <sys/types.h>
#include <bool.h>
#include <libadt/hash_table.h>

#define dprintf(...)	printf(__VA_ARGS__)

typedef struct tmpfs_dentry {
	unsigned long index;	/**< TMPFS node index. */
	link_t dh_link;		/**< Dentries hash table link. */
	struct tmpfs_dentry *sibling;
	struct tmpfs_dentry *child;
	char *name;
	enum {
		TMPFS_NONE,
		TMPFS_FILE,
		TMPFS_DIRECTORY
	} type;
	unsigned lnkcnt;	/**< Link count. */
	size_t size;		/**< File size if type is TMPFS_FILE. */
	void *data;		/**< File content's if type is TMPFS_FILE. */
} tmpfs_dentry_t;

extern fs_reg_t tmpfs_reg;

extern void tmpfs_lookup(ipc_callid_t, ipc_call_t *);
extern void tmpfs_read(ipc_callid_t, ipc_call_t *);
extern void tmpfs_write(ipc_callid_t, ipc_call_t *);
extern void tmpfs_truncate(ipc_callid_t, ipc_call_t *);
extern void tmpfs_destroy(ipc_callid_t, ipc_call_t *);

#endif

/**
 * @}
 */
