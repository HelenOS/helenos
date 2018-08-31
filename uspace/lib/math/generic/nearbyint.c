/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include <assert.h>
#include <math.h>
#include <fenv.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>

static float _roundf_even(float val)
{
	assert(!signbit(val));

	/* Get some special cases out of the way first. */

	if (islessequal(val, 0.5f))
		return 0.0f;

	if (isless(val, 1.5f))
		return 1.0f;

	if (islessequal(val, 2.5f))
		return 2.0f;

	const int exp_bias = FLT_MAX_EXP - 1;
	const int mant_bits = FLT_MANT_DIG - 1;
	const uint32_t mant_mask = (UINT32_C(1) << mant_bits) - 1;

	union {
		float f;
		uint32_t i;
	} u = { .f = val };

	int exp = (u.i >> mant_bits) - exp_bias;
	assert(exp > 0);

	/* Mantissa has no fractional places. */
	if (exp >= mant_bits)
		return val;

	/* Check whether we are rounding up or down. */
	uint32_t first = (UINT32_C(1) << (mant_bits - exp));
	uint32_t midpoint = first >> 1;
	uint32_t frac = u.i & (mant_mask >> exp);

	bool up;
	if (frac == midpoint) {
		up = (u.i & first);
	} else {
		up = (frac > midpoint);
	}

	u.i &= ~(mant_mask >> exp);
	if (up) {
		u.i += first;
	}
	return copysignf(u.f, val);
}

static float _round_even(float val)
{
	assert(!signbit(val));

	/* Get some special cases out of the way first. */

	if (islessequal(val, 0.5))
		return 0.0;

	if (isless(val, 1.5))
		return 1.0;

	if (islessequal(val, 2.5))
		return 2.0;

	const int exp_bias = DBL_MAX_EXP - 1;
	const int mant_bits = DBL_MANT_DIG - 1;
	const uint64_t mant_mask = (UINT64_C(1) << mant_bits) - 1;

	union {
		double f;
		uint64_t i;
	} u = { .f = val };

	int exp = (u.i >> mant_bits) - exp_bias;
	assert(exp > 0);

	/* Mantissa has no fractional places. */
	if (exp >= mant_bits)
		return val;

	/* Check whether we are rounding up or down. */
	uint64_t first = (UINT64_C(1) << (mant_bits - exp));
	uint64_t midpoint = first >> 1;
	uint64_t frac = u.i & (mant_mask >> exp);

	bool up;
	if (frac == midpoint) {
		up = (u.i & first);
	} else {
		up = (frac > midpoint);
	}

	u.i &= ~(mant_mask >> exp);
	if (up) {
		u.i += first;
	}
	return copysignf(u.f, val);
}

/**
 * Rounds its argument to the nearest integer value in floating-point format,
 * using the current rounding direction and without raising the inexact
 * floating-point exception.
 */
float nearbyintf(float val)
{
	switch (fegetround()) {
	case FE_DOWNWARD:
		return floorf(val);
	case FE_UPWARD:
		return ceilf(val);
	case FE_TOWARDZERO:
		return truncf(val);
	case FE_TONEAREST:
		return copysignf(_roundf_even(fabsf(val)), val);
	}

	assert(false);
}

/**
 * Rounds its argument to the nearest integer value in floating-point format,
 * using the current rounding direction and without raising the inexact
 * floating-point exception.
 */
double nearbyint(double val)
{
	switch (fegetround()) {
	case FE_DOWNWARD:
		return floor(val);
	case FE_UPWARD:
		return ceil(val);
	case FE_TOWARDZERO:
		return trunc(val);
	case FE_TONEAREST:
		return copysign(_round_even(fabs(val)), val);
	}

	assert(false);
}

/** @}
 */
