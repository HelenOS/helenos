/*
 * Copyright (c) 2009 Martin Decky
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
