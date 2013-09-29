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
/** @file Miscellaneous standard definitions.
 */

#ifndef POSIX_UNISTD_H_
#define POSIX_UNISTD_H_

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "sys/types.h"
#include "stddef.h"

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

/* Process Termination */
#define _exit exit

extern char *__POSIX_DEF__(optarg);
extern int optind, opterr, optopt;
extern int __POSIX_DEF__(getopt)(int, char * const [], const char *);

/* Environment */
extern char **__POSIX_DEF__(environ);

/* Login Information */
extern char *__POSIX_DEF__(getlogin)(void);
extern int __POSIX_DEF__(getlogin_r)(char *name, size_t namesize);

/* Identifying Terminals */
extern int __POSIX_DEF__(isatty)(int fd);

/* Working Directory */
extern char *__POSIX_DEF__(getcwd)(char *buf, size_t size);
extern int __POSIX_DEF__(chdir)(const char *path);

/* Query Memory Parameters */
extern int __POSIX_DEF__(getpagesize)(void);

/* Process Identification */
extern __POSIX_DEF__(pid_t) __POSIX_DEF__(getpid)(void);
extern __POSIX_DEF__(uid_t) __POSIX_DEF__(getuid)(void);
extern __POSIX_DEF__(gid_t) __POSIX_DEF__(getgid)(void);

/* File Manipulation */
extern int __POSIX_DEF__(close)(int fildes);
extern ssize_t __POSIX_DEF__(read)(int fildes, void *buf, size_t nbyte);
extern ssize_t __POSIX_DEF__(write)(int fildes, const void *buf, size_t nbyte);
extern __POSIX_DEF__(off_t) __POSIX_DEF__(lseek)(int fildes,
    __POSIX_DEF__(off_t) offset, int whence);
extern int __POSIX_DEF__(fsync)(int fildes);
extern int __POSIX_DEF__(ftruncate)(int fildes, __POSIX_DEF__(off_t) length);
extern int __POSIX_DEF__(rmdir)(const char *path);
extern int __POSIX_DEF__(unlink)(const char *path);
extern int __POSIX_DEF__(dup)(int fildes);
extern int __POSIX_DEF__(dup2)(int fildes, int fildes2);

/* Standard Streams */
#undef STDIN_FILENO
#define STDIN_FILENO (fileno(stdin))
#undef STDOUT_FILENO
#define STDOUT_FILENO (fileno(stdout))
#undef STDERR_FILENO
#define STDERR_FILENO (fileno(stderr))

/* File Accessibility */
#undef F_OK
#undef X_OK
#undef W_OK
#undef R_OK
#define	F_OK 0 /* Test for existence. */
#define	X_OK 1 /* Test for execute permission. */
#define	W_OK 2 /* Test for write permission. */
#define	R_OK 4 /* Test for read permission. */
extern int __POSIX_DEF__(access)(const char *path, int amode);

/* System Parameters */
enum {
	_SC_PHYS_PAGES,
	_SC_AVPHYS_PAGES,
	_SC_PAGESIZE,
	_SC_CLK_TCK
};
extern long __POSIX_DEF__(sysconf)(int name);

/* Path Configuration Parameters */
enum {
	_PC_2_SYMLINKS,
	_PC_ALLOC_SIZE_MIN,
	_PC_ASYNC_IO,
	_PC_CHOWN_RESTRICTED,
	_PC_FILESIZEBITS,
	_PC_LINK_MAX,
	_PC_MAX_CANON,
	_PC_MAX_INPUT,
	_PC_NAME_MAX,
	_PC_NO_TRUNC,
	_PC_PATH_MAX,
	_PC_PIPE_BUF,
	_PC_PRIO_IO,
	_PC_REC_INCR_XFER_SIZE,
	_PC_REC_MIN_XFER_SIZE,
	_PC_REC_XFER_ALIGN,
	_PC_SYMLINK_MAX,
	_PC_SYNC_IO,
	_PC_VDISABLE
};
extern long __POSIX_DEF__(pathconf)(const char *path, int name);

/* Creating a Process */
extern __POSIX_DEF__(pid_t) __POSIX_DEF__(fork)(void);

/* Executing a File */
extern int __POSIX_DEF__(execv)(const char *path, char *const argv[]);
extern int __POSIX_DEF__(execvp)(const char *file, char *const argv[]);

/* Creating a Pipe */
extern int __POSIX_DEF__(pipe)(int fildes[2]);

/* Issue alarm signal. */
extern unsigned int __POSIX_DEF__(alarm)(unsigned int);

#endif /* POSIX_UNISTD_H_ */

/** @}
 */
