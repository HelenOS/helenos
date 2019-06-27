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

#include <stddef.h>
#include <sys/types.h>

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

/* Process Termination */
#define _exit exit

/* Standard Streams */
#define STDIN_FILENO (fileno(stdin))
#define STDOUT_FILENO (fileno(stdout))
#define STDERR_FILENO (fileno(stderr))

#define	F_OK 0 /* Test for existence. */
#define	X_OK 1 /* Test for execute permission. */
#define	W_OK 2 /* Test for write permission. */
#define	R_OK 4 /* Test for read permission. */

__C_DECLS_BEGIN;

extern char *optarg;
extern int optind, opterr, optopt;
extern int getopt(int, char *const [], const char *);

/* Environment */
extern char **environ;

/* Sleeping */
extern unsigned int sleep(unsigned int);

/* Login Information */
extern char *getlogin(void);
extern int getlogin_r(char *name, size_t namesize);

/* Identifying Terminals */
extern int isatty(int fd);

/* Working Directory */
extern char *getcwd(char *buf, size_t size);
extern int chdir(const char *path);

/* Query Memory Parameters */
extern int getpagesize(void);

/* Process Identification */
extern pid_t getpid(void);
extern uid_t getuid(void);
extern gid_t getgid(void);

/* File Manipulation */
extern int close(int fildes);
extern ssize_t read(int fildes, void *buf, size_t nbyte);
extern ssize_t write(int fildes, const void *buf, size_t nbyte);
extern int fsync(int fildes);
extern int rmdir(const char *path);
extern int unlink(const char *path);
extern int dup(int fildes);
extern int dup2(int fildes, int fildes2);

#if defined(_LARGEFILE64_SOURCE) || defined(_GNU_SOURCE)
// FIXME: this should just be defined in <sys/types.h>, but for some reason
//        build of coastline binutils on mips32 doesn't see the definition there
typedef int64_t off64_t;

extern off64_t lseek64(int fildes, off64_t offset, int whence);
extern int ftruncate64(int fildes, off64_t length);
#endif

#if _FILE_OFFSET_BITS == 64 && LONG_MAX == INT_MAX
#ifdef __GNUC__
extern off_t lseek(int fildes, off_t offset, int whence) __asm__("lseek64");
extern int ftruncate(int fildes, off_t length) __asm__("ftruncate64");
#else
extern off_t lseek64(int fildes, off_t offset, int whence);
extern int ftruncate64(int fildes, off_t length);
#define lseek lseek64
#define ftruncate ftruncate64
#endif
#else
extern off_t lseek(int fildes, off_t offset, int whence);
extern int ftruncate(int fildes, off_t length);
#endif

/* File Accessibility */
extern int access(const char *path, int amode);

/* System Parameters */
enum {
	_SC_PHYS_PAGES,
	_SC_AVPHYS_PAGES,
	_SC_PAGESIZE,
	_SC_CLK_TCK
};
extern long sysconf(int name);

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
extern long pathconf(const char *path, int name);

/* Creating a Process */
extern pid_t fork(void);

/* Executing a File */
extern int execv(const char *path, char *const argv[]);
extern int execvp(const char *file, char *const argv[]);

/* Creating a Pipe */
extern int pipe(int fildes[2]);

/* Issue alarm signal. */
extern unsigned int alarm(unsigned int);

__C_DECLS_END;

#endif /* POSIX_UNISTD_H_ */

/** @}
 */
