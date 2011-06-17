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

#ifndef POSIX_UNISTD_H_
#define POSIX_UNISTD_H_

#include "libc/unistd.h"
#include "sys/types.h"

/* Process Termination */
#define _exit exit

/* Option Arguments */
extern char *optarg;
extern int optind, opterr, optopt;
extern int getopt(int, char * const [], const char *);

/* Identifying Terminals */
extern int posix_isatty(int fd);

/* Process Identification */
#define getpid task_get_id
extern posix_uid_t posix_getuid(void);
extern posix_gid_t posix_getgid(void);

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
extern int posix_access(const char *path, int amode);

/* System Parameters */
enum {
	_SC_PHYS_PAGES,
	_SC_AVPHYS_PAGES,
	_SC_PAGESIZE,
	_SC_CLK_TCK
};
extern long posix_sysconf(int name);

#ifndef LIBPOSIX_INTERNAL
	#define isatty posix_isatty

	#define getuid posix_getuid
	#define getgid posix_getgid

	#define access posix_access

	#define sysconf posix_sysconf
#endif

#endif /* POSIX_UNISTD_H_ */

/** @}
 */
