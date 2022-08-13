/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_MEM_H_
#define _LIBC_MEM_H_

#include <stddef.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

extern void *memset(void *, int, size_t)
    __attribute__((nonnull(1)));
extern void *memcpy(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));
extern void *memmove(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));
extern int memcmp(const void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));
extern void *memchr(const void *, int, size_t)
    __attribute__((nonnull(1)));

__C_DECLS_END;

#endif

/** @}
 */
