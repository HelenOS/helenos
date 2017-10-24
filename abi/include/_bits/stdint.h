/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

/* Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 */

#ifndef _BITS_STDINT_H_
#define _BITS_STDINT_H_

#include <_bits/macros.h>

typedef __INT8_TYPE__ int8_t;
typedef __INT8_TYPE__ int_least8_t;
typedef __INT8_TYPE__ int_fast8_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __UINT8_TYPE__ uint_least8_t;
typedef __UINT8_TYPE__ uint_fast8_t;

typedef __INT16_TYPE__ int16_t;
typedef __INT16_TYPE__ int_least16_t;
typedef __INT16_TYPE__ int_fast16_t;

typedef __UINT16_TYPE__ uint16_t;
typedef __UINT16_TYPE__ uint_least16_t;
typedef __UINT16_TYPE__ uint_fast16_t;

typedef __INT32_TYPE__ int32_t;
typedef __INT32_TYPE__ int_least32_t;
typedef __INT32_TYPE__ int_fast32_t;

typedef __UINT32_TYPE__ uint32_t;
typedef __UINT32_TYPE__ uint_least32_t;
typedef __UINT32_TYPE__ uint_fast32_t;

typedef __INT64_TYPE__ int64_t;
typedef __INT64_TYPE__ int_least64_t;
typedef __INT64_TYPE__ int_fast64_t;

typedef __UINT64_TYPE__ uint64_t;
typedef __UINT64_TYPE__ uint_least64_t;
typedef __UINT64_TYPE__ uint_fast64_t;

#define INT8_MIN        __INT8_MIN__
#define INT_LEAST8_MIN  __INT8_MIN__
#define INT_FAST8_MIN   __INT8_MIN__

#define INT8_MAX        __INT8_MAX__
#define INT_LEAST8_MAX  __INT8_MAX__
#define INT_FAST8_MAX   __INT8_MAX__

#define UINT8_MIN        __UINT8_C(0)
#define UINT_LEAST8_MIN  __UINT8_C(0)
#define UINT_FAST8_MIN   __UINT8_C(0)

#define UINT8_MAX        __UINT8_MAX__
#define UINT_LEAST8_MAX  __UINT8_MAX__
#define UINT_FAST8_MAX   __UINT8_MAX__

#define INT16_MIN        __INT16_MIN__
#define INT_LEAST16_MIN  __INT16_MIN__
#define INT_FAST16_MIN   __INT16_MIN__

#define INT16_MAX        __INT16_MAX__
#define INT_LEAST16_MAX  __INT16_MAX__
#define INT_FAST16_MAX   __INT16_MAX__

#define UINT16_MIN        __UINT16_C(0)
#define UINT_LEAST16_MIN  __UINT16_C(0)
#define UINT_FAST16_MIN   __UINT16_C(0)

#define UINT16_MAX        __UINT16_MAX__
#define UINT_LEAST16_MAX  __UINT16_MAX__
#define UINT_FAST16_MAX   __UINT16_MAX__

#define INT32_MIN        __INT32_MIN__
#define INT_LEAST32_MIN  __INT32_MIN__
#define INT_FAST32_MIN   __INT32_MIN__

#define INT32_MAX        __INT32_MAX__
#define INT_LEAST32_MAX  __INT32_MAX__
#define INT_FAST32_MAX   __INT32_MAX__

#define UINT32_MIN        __UINT32_C(0)
#define UINT_LEAST32_MIN  __UINT32_C(0)
#define UINT_FAST32_MIN   __UINT32_C(0)

#define UINT32_MAX        __UINT32_MAX__
#define UINT_LEAST32_MAX  __UINT32_MAX__
#define UINT_FAST32_MAX   __UINT32_MAX__

#define INT64_MIN        __INT64_MIN__
#define INT_LEAST64_MIN  __INT64_MIN__
#define INT_FAST64_MIN   __INT64_MIN__

#define INT64_MAX        __INT64_MAX__
#define INT_LEAST64_MAX  __INT64_MAX__
#define INT_FAST64_MAX   __INT64_MAX__

#define UINT64_MIN        __UINT64_C(0)
#define UINT_LEAST64_MIN  __UINT64_C(0)
#define UINT_FAST64_MIN   __UINT64_C(0)

#define UINT64_MAX        __UINT64_MAX__
#define UINT_LEAST64_MAX  __UINT64_MAX__
#define UINT_FAST64_MAX   __UINT64_MAX__

#define INT8_C(x)    __INT8_C(x)
#define INT16_C(x)   __INT16_C(x)
#define INT32_C(x)   __INT32_C(x)
#define INT64_C(x)   __INT64_C(x)
#define UINT8_C(x)   __UINT8_C(x)
#define UINT16_C(x)  __UINT16_C(x)
#define UINT32_C(x)  __UINT32_C(x)
#define UINT64_C(x)  __UINT64_C(x)

typedef __INTPTR_TYPE__  intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;

#define INTPTR_MIN   __INTPTR_MIN__
#define INTPTR_MAX   __INTPTR_MAX__
#define UINTPTR_MIN  __UINTPTR_C(0)
#define UINTPTR_MAX  __UINTPTR_MAX__

typedef __INTMAX_TYPE__  intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

#define INTMAX_MIN   __INTMAX_MIN__
#define INTMAX_MAX   __INTMAX_MAX__
#define UINTMAX_MIN  __UINTMAX_C(0)
#define UINTMAX_MAX  __UINTMAX_MAX__

#define INTMAX_C(x)   __INTMAX_C(x)
#define UINTMAX_C(x)  __UINTMAX_C(x)

#define PTRDIFF_MIN  __PTRDIFF_MIN__
#define PTRDIFF_MAX  __PTRDIFF_MAX__

#define SIZE_MIN  0
#define SIZE_MAX  __SIZE_MAX__

#define WINT_MIN  __WINT_MIN__
#define WINT_MAX  __WINT_MAX__

#include <_bits/WCHAR_MIN.h>
#include <_bits/WCHAR_MAX.h>

/* Use nonstandard 128-bit types if they are supported by the compiler. */

#ifdef __SIZEOF_INT128__
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
#endif

#endif

/** @}
 */
