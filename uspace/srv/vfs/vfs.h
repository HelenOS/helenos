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

/** @addtogroup fs
 * @{
 */ 

#ifndef VFS_VFS_H_
#define VFS_VFS_H_

#include <ipc/ipc.h>
#include <libadt/list.h>
#include <atomic.h>
#include <types.h>

#define dprintf(...)	printf(__VA_ARGS__)

#define VFS_FIRST	FIRST_USER_METHOD

#define IPC_METHOD_TO_VFS_OP(m)	((m) - VFS_FIRST)	

typedef int64_t off_t;

typedef enum {
	VFS_REGISTER = VFS_FIRST,
	VFS_MOUNT,
	VFS_UNMOUNT,
	VFS_LOOKUP,
	VFS_OPEN,
	VFS_CREATE,
	VFS_CLOSE,
	VFS_READ,
	VFS_WRITE,
	VFS_SEEK,
	VFS_LAST,		/* keep this the last member of the enum */
} vfs_request_t;


/**
 * An instance of this structure is associated with a particular FS operation.
 * It tells VFS if the FS supports the operation or maybe if a default one
 * should be used.
 */
typedef enum {
	VFS_OP_NULL = 0,
	VFS_OP_DEFAULT,
	VFS_OP_DEFINED
} vfs_op_t;

#define FS_NAME_MAXLEN	20

/**
 * A structure like this is passed to VFS by each individual FS upon its
 * registration. It assosiates a human-readable identifier with each
 * registered FS. More importantly, through this structure, the FS announces
 * what operations it supports.
 */
typedef struct {
	/** Unique identifier of the fs. */
	char name[FS_NAME_MAXLEN];
	
	/** Operations. */
	vfs_op_t ops[VFS_LAST - VFS_FIRST];
} vfs_info_t;

/**
 * A structure like this will be allocated for each registered file system.
 */
typedef struct {
	link_t fs_link;
	vfs_info_t vfs_info;
	ipcarg_t phone;
} fs_info_t;

/**
 * Instances of this type represent a file system node (e.g. directory, file).
 * They are abstracted away from any file system implementation and contain just
 * enough bits to uniquely identify the object in its file system instance.
 *
 * @note	fs_handle, dev_handle and index are meant to be returned in one
 *		IPC reply.
 */
typedef struct {
	int fs_handle;		/**< Global file system ID. */
	int dev_handle;		/**< Global mount device devno. */
	uint64_t index;		/**< Index of the node on its file system. */
} vfs_node_t;

/**
 * Instances of this type represent an open file. If the file is opened by more
 * than one task, there will be a separate structure allocated for each task.
 */
typedef struct {
	vfs_node_t *node;
	
	/** Number of file handles referencing this file. */
	atomic_t refcnt;

	/** Current position in the file. */
	off_t pos;
} vfs_file_t;

extern link_t fs_head;

extern void vfs_register(ipc_callid_t rid, ipc_call_t *request);

#endif

/**
 * @}
 */
