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

#ifndef POSIX_FCNTL_H_
#define POSIX_FCNTL_H_

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "sys/types.h"
#include "libc/fcntl.h"
#include "errno.h"

/* Mask for file access modes. */
#undef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

/* Dummy compatibility flag */
#undef O_NOCTTY
#define O_NOCTTY 0

/* fcntl commands */
#undef F_DUPFD
#undef F_DUPFD_CLOEXEC
#undef F_GETFD
#undef F_SETFD
#undef F_GETFL
#undef F_SETFL
#undef F_GETOWN
#undef F_SETOWN
#undef F_GETLK
#undef F_SETLK
#undef F_SETLKW
#define F_DUPFD            0 /* Duplicate file descriptor. */
#define F_DUPFD_CLOEXEC    1 /* Same as F_DUPFD but with FD_CLOEXEC flag set. */
#define F_GETFD            2 /* Get file descriptor flags. */
#define F_SETFD            3 /* Set file descriptor flags. */
#define F_GETFL            4 /* Get file status and access flags. */
#define F_SETFL            5 /* Set file status flags. */
#define F_GETOWN           6 /* Get socket owner. */
#define F_SETOWN           7 /* Set socket owner. */
#define F_GETLK            8 /* Get locking information. */
#define F_SETLK            9 /* Set locking information. */
#define F_SETLKW          10 /* Set locking information; wait if blocked. */

/* File descriptor flags used with F_GETFD and F_SETFD. */
#undef FD_CLOEXEC
#define FD_CLOEXEC         1 /* Close on exec. */

extern int __POSIX_DEF__(open)(const char *pathname, int flags, ...);
extern int __POSIX_DEF__(fcntl)(int fd, int cmd, ...);


#endif /* POSIX_FCNTL_H_ */

/** @}
 */
