/*
 * Copyright (c) 2015 Jiri Svoboda
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
/**
 * @file Integer mathematical functions
 */

#include <errno.h>
#include <imath.h>
#include <stdbool.h>
#include <stdint.h>

/** Compute integer power of 10, unsigned 64-bit result.
 *
 * Fast algorithm using binary digits of exp to compute 10^exp in
 * time O(log exp).
 *
 * @param exp Exponent
 * @param res Place to store result
 * @return EOK on success, ERANGE if result does not fit into result type
 */
errno_t ipow10_u64(unsigned exp, uint64_t *res)
{
	uint64_t a;
	uint64_t r;

	r = 1;
	a = 10;
	while (true) {
		if ((exp & 1) != 0) {
			if ((r * a) / a != r)
				return ERANGE;
			r = r * a;
		}

		exp = exp >> 1;
		if (exp == 0)
			break;

		if ((a * a) / a != a)
			return ERANGE;
		a = a * a;
	}

	*res = r;
	return EOK;
}

/** Compute integer base 10 logarithm, unsigned 64-bit argument.
 *
 * For integer v, compute floor(log_10 v). Fast algorithm computing
 * the binary digits of the result r in time O(log r).
 *
 * @param v Value to compute logarithm from
 * @return Logarithm value
 */
unsigned ilog10_u64(uint64_t v)
{
	unsigned b;
	unsigned e;
	uint64_t p10p2[6];
	uint64_t a;
	unsigned r;

	/* Determine largest b such that 10^2^b <= v */
	b = 0;
	e = 1;
	a = 10;
	p10p2[0] = a;

	while (v / a >= a) {
		++b;
		a = a * a;
		e = e + e;
		p10p2[b] = a;
	}

	/* Determine the binary digits of largest e such that 10^e <= v */
	r = 0;
	while (true) {
		if (v >= p10p2[b]) {
			v = v / p10p2[b];
			r = r ^ (1 << b);
		}

		if (b == 0)
			break;
		--b;
	}

	return r;
}

/** @}
 */
