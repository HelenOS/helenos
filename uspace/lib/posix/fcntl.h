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

#ifndef POSIX_FCNTL_H_
#define POSIX_FCNTL_H_

#include "libc/fcntl.h"

/* fcntl commands */
#undef F_DUPFD
#undef F_GETFD
#undef F_SETFD
#undef F_GETFL
#undef F_SETFL
#undef F_GETOWN
#undef F_SETOWN
#define	F_DUPFD	  	0	/* Duplicate file descriptor. */
#define	F_GETFD		1	/* Get file descriptor flags. */
#define	F_SETFD		2	/* Set file descriptor flags. */
#define	F_GETFL		3	/* Get file status flags. */
#define	F_SETFL		4	/* Set file status flags. */
#define F_GETOWN	5	/* Get owner. */
#define F_SETOWN	6	/* Set owner. */

/* File descriptor flags used with F_GETFD and F_SETFD. */
#undef FD_CLOEXEC
#define	FD_CLOEXEC	1	/* Close on exec. */

extern int posix_fcntl(int fd, int cmd, ...);

#ifndef LIBPOSIX_INTERNAL
	#define fcntl posix_fcntl
#endif

#endif /* POSIX_FCNTL_H_ */

/** @}
 */
