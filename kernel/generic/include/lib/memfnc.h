/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_LIB_MEMFNC_H_
#define KERN_LIB_MEMFNC_H_

#include <stddef.h>
#include <cc.h>

#ifdef CONFIG_LTO
#define DO_NOT_DISCARD ATTRIBUTE_USED
#else
#define DO_NOT_DISCARD
#endif

extern void *memset(void *, int, size_t)
    __attribute__((nonnull(1)))
    ATTRIBUTE_OPTIMIZE("-fno-tree-loop-distribute-patterns") DO_NOT_DISCARD;
extern void *memcpy(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)))
    ATTRIBUTE_OPTIMIZE("-fno-tree-loop-distribute-patterns") DO_NOT_DISCARD;
extern int memcmp(const void *, const void *, size_t len)
    __attribute__((nonnull(1, 2)))
    ATTRIBUTE_OPTIMIZE("-fno-tree-loop-distribute-patterns") DO_NOT_DISCARD;

#define alloca(size) __builtin_alloca((size))

#endif

/** @}
 */
