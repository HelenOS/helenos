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

#ifndef VFS_VFS_H_
#define VFS_VFS_H_

#include <async.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <fibril_synch.h>
#include <sys/types.h>
#include <loc.h>
#include <stdbool.h>
#include <ipc/vfs.h>
#include <task.h>

#ifndef dprintf
	#define dprintf(...)
#endif

/**
 * A structure like this will be allocated for each registered file system.
 */
typedef struct {
	link_t fs_link;
	vfs_info_t vfs_info;
	fs_handle_t fs_handle;
	async_sess_t *sess;
} fs_info_t;

/**
 * VFS_PAIR uniquely represents a file system instance.
 */
#define VFS_PAIR \
	fs_handle_t fs_handle; \
	service_id_t service_id;

/**
 * VFS_TRIPLET uniquely identifies a file system node (e.g. directory, file) but
 * doesn't contain any state. For a stateful structure, see vfs_node_t.
 *
 * @note	fs_handle, service_id and index are meant to be returned in one
 *		IPC reply.
 */
#define VFS_TRIPLET \
	VFS_PAIR; \
	fs_index_t index;

typedef struct {
	VFS_PAIR;
} vfs_pair_t;

typedef struct {
	VFS_TRIPLET;
} vfs_triplet_t;

typedef enum vfs_node_type {
	VFS_NODE_UNKNOWN,
	VFS_NODE_FILE,
	VFS_NODE_DIRECTORY,
} vfs_node_type_t;

typedef struct {
	vfs_triplet_t triplet;
	vfs_node_type_t type;
	aoff64_t size;
	unsigned int lnkcnt;
} vfs_lookup_res_t;

/**
 * Instances of this type represent an active, in-memory VFS node and any state
 * which may be associated with it.
 */
typedef struct {
	VFS_TRIPLET;		/**< Identity of the node. */

	/**
	 * Usage counter.  This includes, but is not limited to, all vfs_file_t
	 * structures that reference this node.
	 */
	unsigned refcnt;
	
	/** Number of names this node has in the file system namespace. */
	unsigned lnkcnt;

	ht_link_t nh_link;		/**< Node hash-table link. */

	vfs_node_type_t type;	/**< Partial info about the node type. */

	aoff64_t size;		/**< Cached size if the node is a file. */

	/**
	 * Holding this rwlock prevents modifications of the node's contents.
	 */
	fibril_rwlock_t contents_rwlock;
} vfs_node_t;

/**
 * Instances of this type represent an open file. If the file is opened by more
 * than one task, there will be a separate structure allocated for each task.
 */
typedef struct {
	/** Serializes access to this open file. */
	fibril_mutex_t lock;

	vfs_node_t *node;
	
	/** Number of file handles referencing this file. */
	unsigned refcnt;

	/** Append on write. */
	bool append;

	/** Current absolute position in the file. */
	aoff64_t pos;
} vfs_file_t;

extern fibril_mutex_t nodes_mutex;

extern fibril_condvar_t fs_list_cv;
extern fibril_mutex_t fs_list_lock;
extern list_t fs_list;		/**< List of registered file systems. */

extern fibril_mutex_t fs_mntlist_lock;
extern list_t fs_mntlist;	/**< List of mounted file systems. */

extern vfs_pair_t rootfs;	/**< Root file system. */

/** Each instance of this type describes one path lookup in progress. */
typedef struct {
	link_t	plb_link;	/**< Active PLB entries list link. */
	unsigned index;		/**< Index of the first character in PLB. */
	size_t len;		/**< Number of characters in this PLB entry. */
} plb_entry_t;

extern fibril_mutex_t plb_mutex;/**< Mutex protecting plb and plb_entries. */
extern uint8_t *plb;		/**< Path Lookup Buffer */
extern list_t plb_entries;	/**< List of active PLB entries. */

/** Holding this rwlock prevents changes in file system namespace. */ 
extern fibril_rwlock_t namespace_rwlock;

extern async_exch_t *vfs_exchange_grab(fs_handle_t);
extern void vfs_exchange_release(async_exch_t *);

extern fs_handle_t fs_name_to_handle(unsigned int instance, char *, bool);
extern vfs_info_t *fs_handle_to_info(fs_handle_t);

extern int vfs_lookup_internal(char *, int, vfs_lookup_res_t *,
    vfs_pair_t *, ...);

extern bool vfs_nodes_init(void);
extern vfs_node_t *vfs_node_get(vfs_lookup_res_t *);
extern void vfs_node_put(vfs_node_t *);
extern void vfs_node_forget(vfs_node_t *);
extern unsigned vfs_nodes_refcount_sum_get(fs_handle_t, service_id_t);


#define MAX_OPEN_FILES	128

extern void *vfs_client_data_create(void);
extern void vfs_client_data_destroy(void *);

extern void vfs_pass_handle(task_id_t, task_id_t, int);
extern int vfs_wait_handle_internal(void);

extern vfs_file_t *vfs_file_get(int);
extern void vfs_file_put(vfs_file_t *);
extern int vfs_fd_assign(vfs_file_t *, int);
extern int vfs_fd_alloc(bool desc);
extern int vfs_fd_free(int);

extern void vfs_node_addref(vfs_node_t *);
extern void vfs_node_delref(vfs_node_t *);
extern int vfs_open_node_remote(vfs_node_t *);

extern void vfs_register(ipc_callid_t, ipc_call_t *);
extern void vfs_mount(ipc_callid_t, ipc_call_t *);
extern void vfs_unmount(ipc_callid_t, ipc_call_t *);
extern void vfs_open(ipc_callid_t, ipc_call_t *);
extern void vfs_sync(ipc_callid_t, ipc_call_t *);
extern void vfs_dup(ipc_callid_t, ipc_call_t *);
extern void vfs_close(ipc_callid_t, ipc_call_t *);
extern void vfs_read(ipc_callid_t, ipc_call_t *);
extern void vfs_write(ipc_callid_t, ipc_call_t *);
extern void vfs_seek(ipc_callid_t, ipc_call_t *);
extern void vfs_truncate(ipc_callid_t, ipc_call_t *);
extern void vfs_fstat(ipc_callid_t, ipc_call_t *);
extern void vfs_stat(ipc_callid_t, ipc_call_t *);
extern void vfs_mkdir(ipc_callid_t, ipc_call_t *);
extern void vfs_unlink(ipc_callid_t, ipc_call_t *);
extern void vfs_rename(ipc_callid_t, ipc_call_t *);
extern void vfs_wait_handle(ipc_callid_t, ipc_call_t *);
extern void vfs_get_mtab(ipc_callid_t, ipc_call_t *);
extern void vfs_statfs(ipc_callid_t, ipc_call_t *);

#endif

/**
 * @}
 */
