/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_MEM_H_
#define KERN_MEM_H_

#include <stddef.h>
#include <stdint.h>
#include <cc.h>

#ifdef CONFIG_LTO
#define DO_NOT_DISCARD ATTRIBUTE_USED
#else
#define DO_NOT_DISCARD
#endif

#define memset(dst, val, cnt)  __builtin_memset((dst), (val), (cnt))
#define memcpy(dst, src, cnt)  __builtin_memcpy((dst), (src), (cnt))
#define memcmp(s1, s2, cnt)    __builtin_memcmp((s1), (s2), (cnt))

extern void memsetb(void *, size_t, uint8_t)
    __attribute__((nonnull(1)));
extern void memsetw(void *, size_t, uint16_t)
    __attribute__((nonnull(1)));
extern void *memmove(void *, const void *, size_t)
    __attribute__((nonnull(1, 2))) DO_NOT_DISCARD;

#endif

/** @}
 */
