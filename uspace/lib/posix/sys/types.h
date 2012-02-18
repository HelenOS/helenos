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
/** @file Data types definitions.
 */

#ifndef POSIX_SYS_TYPES_H_
#define POSIX_SYS_TYPES_H_

#include "../libc/sys/types.h"

typedef unsigned int posix_ino_t;
typedef unsigned int posix_nlink_t;
typedef unsigned int posix_uid_t;
typedef unsigned int posix_gid_t;
typedef off64_t posix_off_t;
typedef long posix_blksize_t;
typedef long posix_blkcnt_t;
typedef int64_t posix_pid_t;
typedef sysarg_t posix_dev_t;

/* PThread Types */
typedef struct posix_thread_attr posix_thread_attr_t;

/* Clock Types */
typedef long posix_clock_t;
typedef int posix_clockid_t;

#ifndef LIBPOSIX_INTERNAL
	#define ino_t posix_ino_t
	#define nlink_t posix_nlink_t
	#define uid_t posix_uid_t
	#define gid_t posix_gid_t
	#define off_t posix_off_t
	#define blksize_t posix_blksize_t
	#define blkcnt_t posix_blkcnt_t
	#define pid_t posix_pid_t
	#define dev_t posix_dev_t
	
	#define pthread_attr_t posix_thread_attr_t
	
	#define clock_t posix_clock_t
	#define clockid_t posix_clockid_t
#endif

#endif /* POSIX_SYS_TYPES_H_ */

/** @}
 */
