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
 * Definition of __noreturn.
 *
 * Expands to the most appropriate noreturn attribute for a given language.
 */

#ifndef _BITS_NORETURN_H_
#define _BITS_NORETURN_H_

#ifndef __noreturn

#if (__GNUC__ >= 3) || (defined(__clang__) && __has_attribute(noreturn))
#define __noreturn __attribute__((noreturn))
#else
#define __noreturn
#endif

#endif

#endif

/** @}
 */
