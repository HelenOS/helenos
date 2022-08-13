/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
