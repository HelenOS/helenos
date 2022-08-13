/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 * Define nonstandard 128-bit types if they are supported by the compiler.
 * GCC only provides this on some architectures. We use it on IA-64.
 */

#ifndef _BITS_INT128_T_H_
#define _BITS_INT128_T_H_

#ifdef __SIZEOF_INT128__
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
#endif

#endif

/** @}
 */
