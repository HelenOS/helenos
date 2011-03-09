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

#ifndef PIPEFS_PIPEFS_H_
#define PIPEFS_PIPEFS_H_

#include <libfs.h>
#include <atomic.h>
#include <sys/types.h>
#include <bool.h>
#include <adt/hash_table.h>
#include <fibril_synch.h>

#define PIPEFS_NODE(node)	((node) ? (pipefs_node_t *)(node)->data : NULL)
#define FS_NODE(node)		((node) ? (node)->bp : NULL)

typedef enum {
	PIPEFS_NONE,
	PIPEFS_FILE,
	PIPEFS_DIRECTORY
} pipefs_dentry_type_t;

/* forward declaration */
struct pipefs_node;

typedef struct pipefs_dentry {
	link_t link;		/**< Linkage for the list of siblings. */
	struct pipefs_node *node;/**< Back pointer to PIPEFS node. */
	char *name;		/**< Name of dentry. */
} pipefs_dentry_t;

typedef struct pipefs_node {
	fs_node_t *bp;		/**< Back pointer to the FS node. */
	fs_index_t index;	/**< PIPEFS node index. */
	devmap_handle_t devmap_handle;/**< Device handle. */
	link_t nh_link;		/**< Nodes hash table link. */
	pipefs_dentry_type_t type;
	unsigned lnkcnt;	/**< Link count. */
	/* Following is for nodes of type PIPEFS_FILE */
	fibril_mutex_t data_lock;
	fibril_condvar_t data_available;
	fibril_condvar_t data_consumed;
	aoff64_t start;		/**< File offset where first data block resides */
	uint8_t *data;		/**< Pointer to data buffer */
	size_t data_size;	/**< Number of remaining bytes in the data buffer */
	/* This is for directory */
	link_t cs_head;		/**< Head of child's siblings list. */
} pipefs_node_t;

extern fs_reg_t pipefs_reg;

extern libfs_ops_t pipefs_libfs_ops;

extern bool pipefs_init(void);

extern void pipefs_mounted(ipc_callid_t, ipc_call_t *);
extern void pipefs_mount(ipc_callid_t, ipc_call_t *);
extern void pipefs_unmounted(ipc_callid_t, ipc_call_t *);
extern void pipefs_unmount(ipc_callid_t, ipc_call_t *);
extern void pipefs_lookup(ipc_callid_t, ipc_call_t *);
extern void pipefs_read(ipc_callid_t, ipc_call_t *);
extern void pipefs_write(ipc_callid_t, ipc_call_t *);
extern void pipefs_truncate(ipc_callid_t, ipc_call_t *);
extern void pipefs_stat(ipc_callid_t, ipc_call_t *);
extern void pipefs_close(ipc_callid_t, ipc_call_t *);
extern void pipefs_destroy(ipc_callid_t, ipc_call_t *);
extern void pipefs_open_node(ipc_callid_t, ipc_call_t *);
extern void pipefs_sync(ipc_callid_t, ipc_call_t *);


#endif

/**
 * @}
 */
