/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "fcntl.h"

#include "libc/unistd.h"
#include "libc/vfs/vfs.h"
#include "errno.h"

/**
 * Performs set of operations on the opened files.
 *
 * @param fd File descriptor of the opened file.
 * @param cmd Operation to carry out.
 * @return Non-negative on success. Might have special meaning corresponding
 *     to the requested operation.
 */
int posix_fcntl(int fd, int cmd, ...)
{
	int rc;
	int flags;

	switch (cmd) {
	case F_DUPFD:
	case F_DUPFD_CLOEXEC: /* FD_CLOEXEC is not supported. */
		/* VFS does not provide means to express constraints on the new
		 * file descriptor so the third argument is ignored. */

		/* Retrieve node triplet corresponding to the file descriptor. */
		/* Empty statement after label. */;
		fdi_node_t node;
		rc = fd_node(fd, &node);
		if (rc != EOK) {
			errno = -rc;
			return -1;
		}

		/* Reopen the node so the fresh file descriptor is generated. */
		int newfd = open_node(&node, 0);
		if (newfd < 0) {
			errno = -newfd;
			return -1;
		}

		/* Associate the newly generated descriptor to the file description
		 * of the old file descriptor. Just reopened node will be automatically
		 * closed. */
		rc = dup2(fd, newfd);
		if (rc != EOK) {
			errno = -rc;
			return -1;
		}

		return newfd;
	case F_GETFD:
		/* FD_CLOEXEC is not supported. There are no other flags. */
		return 0;
	case F_SETFD:
		/* FD_CLOEXEC is not supported. Ignore arguments and report success. */
		return 0;
	case F_GETFL:
		/* File status flags (i.e. O_APPEND) are currently private to the
		 * VFS server so it cannot be easily retrieved. */
		flags = 0;
		/* File access flags are currently not supported for file descriptors.
		 * Provide full access. */
		flags |= O_RDWR;
		return flags;
	case F_SETFL:
		/* File access flags are currently not supported for file descriptors.
		 * Ignore arguments and report success. */
		return 0;
	case F_GETOWN:
	case F_SETOWN:
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/* Signals (SIGURG) and file locks are not supported. */
		errno = ENOTSUP;
		return -1;
	default:
		/* Unknown command */
		errno = EINVAL;
		return -1;
	}
}

/** @}
 */
