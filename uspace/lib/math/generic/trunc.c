/*
 * Copyright (c) 2015 Jiri Svoboda
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup libmath
 * @{
 */
/** @file
 */

#include <mathtypes.h>
#include <trunc.h>

/** Truncate fractional part (round towards zero)
 *
 * Truncate the fractional part of IEEE 754 single
 * precision floating point number by zeroing fraction
 * bits, effectively rounding the number towards zero
 * to the nearest whole number.
 *
 * If the argument is infinity or NaN, an exception
 * should be indicated. This is not implemented yet.
 *
 * @param val Floating point number.
 *
 * @return Number rounded towards zero.
 *
 */
float32_t float32_trunc(float32_t val)
{
	float32_u v;
	int32_t exp;

	v.val = val;
	exp = v.data.parts.exp - FLOAT32_BIAS;

	if (exp < 0) {
		/* -1 < val < 1 => result is +0 or -0 */
		v.data.parts.exp = 0;
		v.data.parts.fraction = 0;
	} else if (exp >= FLOAT32_FRACTION_SIZE) {
		if (exp == 1024) {
			/* val is +inf, -inf or NaN => trigger an exception */
			// FIXME TODO
		}

		/* All bits in val are relevant for the result */
	} else {
		/* Truncate irrelevant fraction bits */
		v.data.parts.fraction &= ~(UINT32_C(0x007fffff) >> exp);
	}

	return v.val;
}

/** Truncate fractional part (round towards zero)
 *
 * Truncate the fractional part of IEEE 754 double
 * precision floating point number by zeroing fraction
 * bits, effectively rounding the number towards zero
 * to the nearest whole number.
 *
 * If the argument is infinity or NaN, an exception
 * should be indicated. This is not implemented yet.
 *
 * @param val Floating point number.
 *
 * @return Number rounded towards zero.
 *
 */
float64_t float64_trunc(float64_t val)
{
	float64_u v;
	int32_t exp;

	v.val = val;
	exp = v.data.parts.exp - FLOAT64_BIAS;

	if (exp < 0) {
		/* -1 < val < 1 => result is +0 or -0 */
		v.data.parts.exp = 0;
		v.data.parts.fraction = 0;
	} else if (exp >= FLOAT64_FRACTION_SIZE) {
		if (exp == 1024) {
			/* val is +inf, -inf or NaN => trigger an exception */
			// FIXME TODO
		}

		/* All bits in val are relevant for the result */
	} else {
		/* Truncate irrelevant fraction bits */
		v.data.parts.fraction &= ~(UINT64_C(0x000fffffffffffff) >> exp);
	}

	return v.val;
}

/** @}
 */
