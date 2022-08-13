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

#ifndef _LIBC_GCDLCM_H_
#define _LIBC_GCDLCM_H_

#include <stddef.h>
#include <stdint.h>

#define DECLARE_GCD(type, name) \
	static inline type name(type a, type b) \
	{ \
		if (a == 0) \
			return b; \
		 \
		while (b != 0) { \
			if (a > b) \
				a -= b; \
			else \
				b -= a; \
		} \
		 \
		return a; \
	}

#define DECLARE_LCM(type, name, gcd) \
	static inline type name(type a, type b) \
	{ \
		return (a * b) / gcd(a, b); \
	}

DECLARE_GCD(uint32_t, gcd32);
DECLARE_GCD(uint64_t, gcd64);
DECLARE_GCD(size_t, gcd);

DECLARE_LCM(uint32_t, lcm32, gcd32);
DECLARE_LCM(uint64_t, lcm64, gcd64);
DECLARE_LCM(size_t, lcm, gcd);

#endif

/** @}
 */
