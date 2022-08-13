/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
