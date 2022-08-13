/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Memory management declarations.
 */

#ifndef POSIX_SYS_MMAN_H_
#define POSIX_SYS_MMAN_H_

#include <sys/types.h>
#include <_bits/decls.h>

#define MAP_FAILED  ((void *) -1)

#define MAP_SHARED     (1 << 0)
#define MAP_PRIVATE    (1 << 1)
#define MAP_FIXED      (1 << 2)
#define MAP_ANONYMOUS  (1 << 3)
#define MAP_ANON       MAP_ANONYMOUS

#define PROT_NONE   0
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4

__C_DECLS_BEGIN;

extern void *mmap(void *start, size_t length, int prot, int flags, int fd,
    off_t offset);
extern int munmap(void *start, size_t length);

__C_DECLS_END;

#endif /* POSIX_SYS_MMAN_H_ */

/** @}
 */
