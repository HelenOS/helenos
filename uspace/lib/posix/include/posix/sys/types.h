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

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "libc/sys/types.h"
#include "libc/sys/time.h"

typedef unsigned int __POSIX_DEF__(ino_t);
typedef unsigned int __POSIX_DEF__(nlink_t);
typedef unsigned int __POSIX_DEF__(uid_t);
typedef unsigned int __POSIX_DEF__(gid_t);
typedef off64_t __POSIX_DEF__(off_t);
typedef long __POSIX_DEF__(blksize_t);
typedef long __POSIX_DEF__(blkcnt_t);
typedef int64_t __POSIX_DEF__(pid_t);
typedef sysarg_t __POSIX_DEF__(dev_t);

/* PThread Types */
typedef struct __POSIX_DEF__(thread_attr) __POSIX_DEF__(thread_attr_t);

/* Clock Types */
typedef long __POSIX_DEF__(clock_t);
typedef int __POSIX_DEF__(clockid_t);


#endif /* POSIX_SYS_TYPES_H_ */

/** @}
 */
