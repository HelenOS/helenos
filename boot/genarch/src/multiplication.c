/*
 * Copyright (c) 2009 Josef Cejka
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

/**
 * @file
 * SW implementation of 32 and 64 bit multiplication.
 */

#include <genarch/multiplication.h>
#include <stdint.h>

/** Set 1 to return INT64_MAX or INT64_MIN on overflow */
#ifndef SOFTINT_CHECK_OF
#define SOFTINT_CHECK_OF  0
#endif

/** Multiply two integers and return long long as result.
 *
 * This function is overflow safe.
 *
 */
static unsigned long long mul(unsigned int a, unsigned int b)
{
	unsigned int a1 = a >> 16;
	unsigned int a2 = a & UINT16_MAX;
	unsigned int b1 = b >> 16;
	unsigned int b2 = b & UINT16_MAX;

	unsigned long long t1 = a1 * b1;
	unsigned long long t2 = a1 * b2;
	t2 += a2 * b1;
	unsigned long long t3 = a2 * b2;

	t3 = (((t1 << 16) + t2) << 16) + t3;

	return t3;
}

/** Emulate multiplication of two 64-bit long long integers.
 *
 */
long long __muldi3 (long long a, long long b)
{
	char neg = 0;

	if (a < 0) {
		neg = !neg;
		a = -a;
	}

	if (b < 0) {
		neg = !neg;
		b = -b;
	}

	unsigned long long a1 = a >> 32;
	unsigned long long b1 = b >> 32;

	unsigned long long a2 = a & (UINT32_MAX);
	unsigned long long b2 = b & (UINT32_MAX);

	if (SOFTINT_CHECK_OF && (a1 != 0) && (b1 != 0)) {
		/* Error (overflow) */
		return (neg ? INT64_MIN : INT64_MAX);
	}

	/* (if OF checked) a1 or b1 is zero => result fits in 64 bits,
	 * no need to another overflow check
	 */
	unsigned long long t1 = mul(a1, b2) + mul(b1, a2);

	if ((SOFTINT_CHECK_OF) && (t1 > UINT32_MAX)) {
		/* Error (overflow) */
		return (neg ? INT64_MIN : INT64_MAX);
	}

	t1 = t1 << 32;
	unsigned long long t2 = mul(a2, b2);
	t2 += t1;

	/* t2 & (1ull << 63) - if this bit is set in unsigned long long,
	 * result does not fit in signed one
	 */
	if ((SOFTINT_CHECK_OF) && ((t2 < t1) || (t2 & (1ull << 63)))) {
		/* Error (overflow) */
		return (neg ? INT64_MIN : INT64_MAX);
	}

	long long result = t2;
	if (neg)
		result = -result;

	return result;
}

/** @}
 */
