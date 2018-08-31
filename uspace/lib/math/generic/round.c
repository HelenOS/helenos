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

#include <math.h>
#include <float.h>
#include <stdint.h>

/**
 * Rounds its argument to the nearest integer value in floating-point format,
 * rounding halfway cases away from zero, regardless of the current rounding
 * direction.
 */
float roundf(float val)
{
	const int exp_bias = FLT_MAX_EXP - 1;
	const int mant_bits = FLT_MANT_DIG - 1;

	union {
		float f;
		uint32_t i;
	} u = { .f = fabsf(val) };

	int exp = (u.i >> mant_bits) - exp_bias;

	/* If value is less than 0.5, return zero with appropriate sign. */
	if (exp < -1)
		return copysignf(0.0f, val);

	/* If exponent is exactly mant_bits, adding 0.5 could change the result. */
	if (exp >= mant_bits)
		return val;

	/* Use trunc with adjusted value to do the rounding. */
	return copysignf(truncf(u.f + 0.5), val);
}

/**
 * Rounds its argument to the nearest integer value in floating-point format,
 * rounding halfway cases away from zero, regardless of the current rounding
 * direction.
 */
double round(double val)
{
	const int exp_bias = DBL_MAX_EXP - 1;
	const int mant_bits = DBL_MANT_DIG - 1;

	union {
		double f;
		uint64_t i;
	} u = { .f = fabs(val) };

	int exp = ((int)(u.i >> mant_bits)) - exp_bias;

	/* If value is less than 0.5, return zero with appropriate sign. */
	if (exp < -1)
		return copysign(0.0, val);

	/* If exponent is exactly mant_bits, adding 0.5 could change the result. */
	if (exp >= mant_bits)
		return val;

	/* Use trunc with adjusted value to do the rounding. */
	return copysign(trunc(u.f + 0.5), val);
}

/** @}
 */
