/*
 * Copyright (c) 2017 Jiri Svoboda
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

#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_

#ifdef __GNUC__
/*
 * Default to compiler's version of limits.h, so we don't have to care about
 * language version and whatnot.
 */
#include_next <limits.h>
#else

/* HelenOS requires 8-bit char and two's complement arithmetic. */
#define CHAR_BIT    8
#define SCHAR_MAX   0x7f
#define SCHAR_MIN   (-SCHAR_MAX - 1)
#define SHRT_MAX    0x7fff
#define SHRT_MIN    (-SHRT_MAX - 1)
#define INT_MAX     0x7fffffff
#define INT_MIN     (-INT_MAX - 1)
#define LLONG_MAX   0x7fffffffffffffffll
#define LLONG_MIN   (-LLONG_MAX - 1)

#define UCHAR_MAX   0xff
#define USHRT_MAX   0xffff
#define UINT_MAX    0xffffffffu
#define ULLONG_MAX  0xffffffffffffffffull

#if __STDC_VERSION__ >= 201112L
_Static_assert(((char)-1) < 0, "char should be signed");
#endif

#define CHAR_MAX  SCHAR_MAX
#define CHAR_MIN  SCHAR_MIN

#ifdef __32_BITS__
#define LONG_MAX   0x7fffffffl
#define LONG_MIN   (-LONG_MAX - 1)
#define ULONG_MAX  0xfffffffful
#endif

#ifdef __64_BITS__
#define LONG_MAX   0x7fffffffffffffffl
#define LONG_MIN   (-LONG_MAX - 1)
#define ULONG_MAX  0xfffffffffffffffful
#endif

#endif  /* !defined(__GNUC__) */

#undef MB_LEN_MAX
#define MB_LEN_MAX 4

#ifdef _HELENOS_SOURCE
#define UCHAR_MIN   0
#define USHRT_MIN   0
#define UINT_MIN    (0u)
#define ULONG_MIN   (0ul)
#define ULLONG_MIN  (0ull)
#define SSIZE_MIN   INTPTR_MIN
#define UINT8_MIN   0
#define UINT16_MIN  0
#define UINT32_MIN  0
#define UINT64_MIN  0
#endif

#if defined(_HELENOS_SOURCE) || defined(_POSIX_SOURCE) || \
    defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_GNU_SOURCE) || defined(_BSD_SOURCE)

#define SSIZE_MAX  INTPTR_MAX
#define NAME_MAX   255

#endif

/* GCC's <limits.h> doesn't define these for C++11, even though it should. */
#if __cplusplus >= 201103L
#ifndef LLONG_MAX
#define LLONG_MAX  0x7fffffffffffffffll
#endif
#ifndef LLONG_MIN
#define LLONG_MIN  (-LLONG_MAX - 1)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX  0xffffffffffffffffull
#endif
#endif

#endif

/** @}
 */
