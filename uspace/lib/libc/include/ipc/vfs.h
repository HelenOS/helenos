/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef LIBC_IPC_VFS_H_
#define LIBC_IPC_VFS_H_

#include <sys/types.h>
#include <ipc/ipc.h>

#define FS_NAME_MAXLEN  20
#define MAX_PATH_LEN    (64 * 1024)
#define PLB_SIZE        (2 * MAX_PATH_LEN)

/* Basic types. */
typedef int16_t fs_handle_t;
typedef uint32_t fs_index_t;

/**
 * A structure like this is passed to VFS by each individual FS upon its
 * registration. It assosiates a human-readable identifier with each
 * registered FS.
 */
typedef struct {
	/** Unique identifier of the fs. */
	char name[FS_NAME_MAXLEN + 1];
} vfs_info_t;

typedef enum {
	VFS_OPEN_NODE = IPC_FIRST_USER_METHOD,
	VFS_READ,
	VFS_WRITE,
	VFS_TRUNCATE,
	VFS_MOUNT,
	VFS_UNMOUNT,
	VFS_DEVICE,
	VFS_SYNC,
	VFS_CLOSE,
	VFS_LAST_CMN  /* keep this the last member of this enum */
} vfs_request_cmn_t;

typedef enum {
	VFS_LOOKUP = VFS_LAST_CMN,
	VFS_MOUNTED,
	VFS_DESTROY,
	VFS_LAST_CLNT  /* keep this the last member of this enum */
} vfs_request_clnt_t;

typedef enum {
	VFS_REGISTER = VFS_LAST_CMN,
	VFS_OPEN,
	VFS_SEEK,
	VFS_MKDIR,
	VFS_UNLINK,
	VFS_RENAME,
	VFS_NODE,
	VFS_LAST_SRV  /* keep this the last member of this enum */
} vfs_request_srv_t;

/*
 * Lookup flags.
 */

/**
 * No lookup flags used.
 */
#define L_NONE  0

/**
 * Lookup will succeed only if the object is a regular file.  If L_CREATE is
 * specified, an empty file will be created. This flag is mutually exclusive
 * with L_DIRECTORY.
 */
#define L_FILE  1

/**
 * Lookup wil succeed only if the object is a directory. If L_CREATE is
 * specified, an empty directory will be created. This flag is mutually
 * exclusive with L_FILE.
 */
#define L_DIRECTORY  2

/**
 * When used with L_CREATE, L_EXCLUSIVE will cause the lookup to fail if the
 * object already exists. L_EXCLUSIVE is implied when L_DIRECTORY is used.
 */
#define L_EXCLUSIVE  4

/**
 * L_CREATE is used for creating both regular files and directories.
 */
#define L_CREATE  8

/**
 * L_LINK is used for linking to an already existing nodes.
 */
#define L_LINK  16

/**
 * L_UNLINK is used to remove leaves from the file system namespace. This flag
 * cannot be passed directly by the client, but will be set by VFS during
 * VFS_UNLINK.
 */
#define L_UNLINK  32

/**
 * L_OPEN is used to indicate that the lookup operation is a part of VFS_OPEN
 * call from the client. This means that the server might allocate some
 * resources for the opened file. This flag cannot be passed directly by the
 * client.
 */
#define L_OPEN  64

#endif

/** @}
 */
