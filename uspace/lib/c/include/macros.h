/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_MACROS_H_
#define _LIBC_MACROS_H_

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define mabs(a)    ((a) >= 0 ? (a) : -(a))

#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))

#define KiB2SIZE(kb)  ((kb) << 10)
#define MiB2SIZE(mb)  ((mb) << 20)

#define STRING(arg)      STRING_ARG(arg)
#define STRING_ARG(arg)  #arg

#define LOWER32(arg)  (((uint64_t) (arg)) & 0xffffffff)
#define UPPER32(arg)  (((((uint64_t) arg)) >> 32) & 0xffffffff)

#define MERGE_LOUP32(lo, up) \
	((((uint64_t) (lo)) & 0xffffffff) \
	    | ((((uint64_t) (up)) & 0xffffffff) << 32))

#define _paddname(line)     PADD_ ## line ## __
#define _padd(width, line, n)  uint ## width ## _t _paddname(line) [n]

#define PADD32(n)  _padd(32, __LINE__, n)
#define PADD16(n)  _padd(16, __LINE__, n)
#define PADD8(n)   _padd(8, __LINE__, n)

#endif

/** @}
 */
