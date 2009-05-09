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

#ifndef dprintf
#define dprintf(...)	printf(__VA_ARGS__)
#endif

#define TMPFS_NODE(node)	((node) ? (tmpfs_node_t *)(node)->data : NULL)
#define FS_NODE(node)		((node) ? (node)->bp : NULL)

typedef enum {
	TMPFS_NONE,
	TMPFS_FILE,
	TMPFS_DIRECTORY
} tmpfs_dentry_type_t;

/* forward declaration */
struct tmpfs_node;

typedef struct tmpfs_dentry {
	link_t link;		/**< Linkage for the list of siblings. */
	struct tmpfs_node *node;/**< Back pointer to TMPFS node. */
	char *name;		/**< Name of dentry. */
} tmpfs_dentry_t;

typedef struct tmpfs_node {
	fs_node_t *bp;		/**< Back pointer to the FS node. */
	fs_index_t index;	/**< TMPFS node index. */
	dev_handle_t dev_handle;/**< Device handle. */
	link_t nh_link;		/**< Nodes hash table link. */
	tmpfs_dentry_type_t type;
	unsigned lnkcnt;	/**< Link count. */
	size_t size;		/**< File size if type is TMPFS_FILE. */
	void *data;		/**< File content's if type is TMPFS_FILE. */
	link_t cs_head;		/**< Head of child's siblings list. */
} tmpfs_node_t;

extern fs_reg_t tmpfs_reg;

extern libfs_ops_t tmpfs_libfs_ops;

extern bool tmpfs_init(void);

extern void tmpfs_mounted(ipc_callid_t, ipc_call_t *);
extern void tmpfs_mount(ipc_callid_t, ipc_call_t *);
extern void tmpfs_lookup(ipc_callid_t, ipc_call_t *);
extern void tmpfs_read(ipc_callid_t, ipc_call_t *);
extern void tmpfs_write(ipc_callid_t, ipc_call_t *);
extern void tmpfs_truncate(ipc_callid_t, ipc_call_t *);
extern void tmpfs_destroy(ipc_callid_t, ipc_call_t *);

extern bool tmpfs_restore(dev_handle_t);

#endif

/**
 * @}
 */
