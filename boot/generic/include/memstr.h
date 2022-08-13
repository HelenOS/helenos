/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_MEMSTR_H_
#define BOOT_MEMSTR_H_

#include <stddef.h>

extern void *memcpy(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)))
    __attribute__((optimize("-fno-tree-loop-distribute-patterns")));
extern void *memset(void *, int, size_t)
    __attribute__((nonnull(1)))
    __attribute__((optimize("-fno-tree-loop-distribute-patterns")));
extern void *memmove(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));

#endif

/** @}
 */
