/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2018 Jiri Svoboda
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

#if !__STDC_HOSTED__
#define memset(dst, val, cnt)  __builtin_memset((dst), (val), (cnt))
#define memcpy(dst, src, cnt)  __builtin_memcpy((dst), (src), (cnt))
#define memcmp(s1, s2, cnt)    __builtin_memcmp((s1), (s2), (cnt))
#define memmove(dst, src, cnt)  __builtin_memmove((dst), (src), (cnt))
#define memchr(s, c, cnt)  __builtin_memchr((s), (c), (cnt))
#endif

#endif

/** @}
 */
