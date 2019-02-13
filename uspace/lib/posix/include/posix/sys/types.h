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

#include <stddef.h>
#include <stdint.h>
#include <_bits/ssize_t.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

typedef unsigned int ino_t;
typedef unsigned int nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long blksize_t;
typedef long blkcnt_t;
typedef int pid_t;
typedef unsigned long dev_t;
typedef unsigned int mode_t;

#if _FILE_OFFSET_BITS == 64
typedef int64_t off_t;
#else
typedef long off_t;
#endif

#if defined(_LARGEFILE64_SOURCE) || defined(_GNU_SOURCE)
typedef int64_t off64_t;
#endif

/* PThread Types */
typedef struct thread_attr thread_attr_t;

/* Clock Types */
typedef int clockid_t;

typedef long suseconds_t;

__C_DECLS_END;

#endif /* POSIX_SYS_TYPES_H_ */

/** @}
 */
