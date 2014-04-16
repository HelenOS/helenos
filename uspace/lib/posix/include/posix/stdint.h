/*
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
/** @file Integer types.
 */

#ifndef POSIX_STDINT_H_
#define POSIX_STDINT_H_

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "libc/stdint.h"

#undef INT8_MAX
#undef INT8_MIN
#define INT8_MAX  127
#define INT8_MIN  (-128)

#undef UINT8_MAX
#undef UINT8_MIN
#define UINT8_MAX  255
#define UINT8_MIN  0

#undef INT16_MAX
#undef INT16_MIN
#define INT16_MAX  32767
#define INT16_MIN  (-32768)

#undef UINT16_MAX
#undef UINT16_MIN
#define UINT16_MAX  65535
#define UINT16_MIN  0

#undef INT32_MAX
#undef INT32_MIN
#define INT32_MAX  2147483647
#define INT32_MIN  (-INT32_MAX - 1)

#undef UINT32_MAX
#undef UINT32_MIN
#define UINT32_MAX  4294967295U
#define UINT32_MIN  0U

#undef INT64_MAX
#undef INT64_MIN
#define INT64_MAX  9223372036854775807LL
#define INT64_MIN  (-INT64_MAX - 1LL)

#undef UINT64_MAX
#undef  UINT64_MIN
#define UINT64_MAX  18446744073709551615ULL
#define UINT64_MIN  0ULL

#undef OFF64_MAX
#undef OFF64_MIN
#define OFF64_MAX  INT64_MAX
#define OFF64_MIN  INT64_MIN

#undef AOFF64_MAX
#undef AOFF64_MIN
#define AOFF64_MAX  UINT64_MAX
#define AOFF64_MIN  UINT64_MIN

#undef INTMAX_MIN
#undef INTMAX_MAX
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX

#undef UINTMAX_MIN
#undef UINTMAX_MAX
#define UINTMAX_MIN UINT64_MIN
#define UINTMAX_MAX UINT64_MAX

#include "libc/sys/types.h"

typedef int64_t __POSIX_DEF__(intmax_t);
typedef uint64_t __POSIX_DEF__(uintmax_t);


/*
 * Fast* and least* integer types.
 *
 * The definitions below are definitely safe if not the best.
 */
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;

#endif /* POSIX_STDINT_H_ */

/** @}
 */
