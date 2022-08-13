/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
