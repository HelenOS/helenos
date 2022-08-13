/*
 * SPDX-FileCopyrightText: 2017 CZ.NIC, z.s.p.o.
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
 */

#ifndef _BITS_WCHAR_LIMITS_H_
#define _BITS_WCHAR_LIMITS_H_

/* wchar_t should always be int32_t in HelenOS. */

#include <_bits/wchar_t.h>

#ifndef __cplusplus
_Static_assert(((wchar_t) -1) < 0, "wchar_t is not int32_t");
_Static_assert(sizeof(wchar_t) == 4, "wchar_t is not int32_t");
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX  0x7fffffff
#endif
#ifndef WCHAR_MIN
#define WCHAR_MIN  (-WCHAR_MIN - 1)
#endif

#endif

/** @}
 */
