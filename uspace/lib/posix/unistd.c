/*
 * Copyright (c) 2011 Jiri Zarevucky
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
#include "unistd.h"
#include <task.h>

/* Array of environment variable strings (NAME=VALUE). */
char **posix_environ = NULL;

/**
 * Get current user name.
 */
char *posix_getlogin(void)
{
	// TODO
	return (char *) "user";
}

/**
 * Get current user name.
 *
 * @param name Pointer to a user supplied buffer.
 * @param namesize Length of the buffer.
 * @return
 */
int posix_getlogin_r(char *name, size_t namesize)
{
	// TODO
	not_implemented();
}

/**
 * Dummy function. Always returns false, because there is no easy way to find
 * out under HelenOS.
 *
 * @param fd
 * @return Always false.
 */
int posix_isatty(int fd)
{
	return false;
}

/**
 * 
 * @return
 */
int posix_getpagesize(void)
{
	return getpagesize();
}

/**
 * 
 * @return
 */
posix_pid_t posix_getpid(void)
{
	return task_get_id();
}

/**
 *
 * @return
 */
posix_uid_t posix_getuid(void)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @return
 */
posix_gid_t posix_getgid(void)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param path
 * @param amode
 * @return
 */
int posix_access(const char *path, int amode)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param name
 * @return
 */
long posix_sysconf(int name)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param path
 * @param name
 * @return
 */
long posix_pathconf(const char *path, int name)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @return
 */
posix_pid_t posix_fork(void)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @param path
 * @param argv
 * @return
 */
int posix_execv(const char *path, char *const argv[])
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @param file
 * @param argv
 * @return
 */
int posix_execvp(const char *file, char *const argv[])
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @param fildes
 * @return
 */
int posix_pipe(int fildes[2])
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/** @}
 */
