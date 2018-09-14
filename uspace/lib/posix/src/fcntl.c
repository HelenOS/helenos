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
/** @file File control.
 */

#include "internal/common.h"
#include <fcntl.h>
#include <vfs/vfs.h>
#include <errno.h>

/**
 * Performs set of operations on the opened files.
 *
 * @param fd File descriptor of the opened file.
 * @param cmd Operation to carry out.
 * @return Non-negative on success. Might have special meaning corresponding
 *     to the requested operation.
 */
int fcntl(int fd, int cmd, ...)
{
	int flags;

	switch (cmd) {
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
		/*
		 * VFS currently does not provide the functionality to duplicate
		 * opened file descriptor.
		 */
		// FIXME: implement this once dup() is available
		errno = ENOTSUP;
		return -1;
	case F_GETFD:
		/* FD_CLOEXEC is not supported. There are no other flags. */
		return 0;
	case F_SETFD:
		/* FD_CLOEXEC is not supported. Ignore arguments and report success. */
		return 0;
	case F_GETFL:
		/*
		 * File status flags (i.e. O_APPEND) are currently private to the
		 * VFS server so it cannot be easily retrieved.
		 */
		flags = 0;
		/*
		 * File access flags are currently not supported for file descriptors.
		 * Provide full access.
		 */
		flags |= O_RDWR;
		return flags;
	case F_SETFL:
		/*
		 * File access flags are currently not supported for file descriptors.
		 * Ignore arguments and report success.
		 */
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

/**
 * Open, possibly create, a file.
 *
 * @param pathname Path to the file.
 * @param posix_flags Access mode flags.
 */
int open(const char *pathname, int posix_flags, ...)
{
	mode_t posix_mode = 0;
	if (posix_flags & O_CREAT) {
		va_list args;
		va_start(args, posix_flags);
		posix_mode = va_arg(args, mode_t);
		va_end(args);
		(void) posix_mode;
	}

	if (((posix_flags & (O_RDONLY | O_WRONLY | O_RDWR)) == 0) ||
	    ((posix_flags & (O_RDONLY | O_WRONLY)) == (O_RDONLY | O_WRONLY)) ||
	    ((posix_flags & (O_RDONLY | O_RDWR)) == (O_RDONLY | O_RDWR)) ||
	    ((posix_flags & (O_WRONLY | O_RDWR)) == (O_WRONLY | O_RDWR))) {
		errno = EINVAL;
		return -1;
	}

	int flags = WALK_REGULAR;
	if (posix_flags & O_CREAT) {
		if (posix_flags & O_EXCL)
			flags |= WALK_MUST_CREATE;
		else
			flags |= WALK_MAY_CREATE;
	}

	int mode =
	    ((posix_flags & O_RDWR) ? MODE_READ | MODE_WRITE : 0) |
	    ((posix_flags & O_RDONLY) ? MODE_READ : 0) |
	    ((posix_flags & O_WRONLY) ? MODE_WRITE : 0) |
	    ((posix_flags & O_APPEND) ? MODE_APPEND : 0);

	int file;

	if (failed(vfs_lookup(pathname, flags, &file)))
		return -1;

	if (failed(vfs_open(file, mode))) {
		vfs_put(file);
		return -1;
	}

	if (posix_flags & O_TRUNC) {
		if (posix_flags & (O_RDWR | O_WRONLY)) {
			if (failed(vfs_resize(file, 0))) {
				vfs_put(file);
				return -1;
			}
		}
	}

	return file;
}

/** @}
 */
