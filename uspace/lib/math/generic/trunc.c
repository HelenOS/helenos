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
float truncf(float val)
{
	const int exp_bias = FLT_MAX_EXP - 1;
	const int mant_bits = FLT_MANT_DIG - 1;
	const uint32_t mant_mask = (UINT32_C(1) << mant_bits) - 1;

	union {
		float f;
		uint32_t i;
	} u = { .f = fabsf(val) };

	int exp = (u.i >> mant_bits) - exp_bias;

	/* If value is less than one, return zero with appropriate sign. */
	if (exp < 0)
		return copysignf(0.0f, val);

	if (exp >= mant_bits)
		return val;

	/* Truncate irrelevant fraction bits */
	u.i &= ~(mant_mask >> exp);
	return copysignf(u.f, val);
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
double trunc(double val)
{
	const int exp_bias = DBL_MAX_EXP - 1;
	const int mant_bits = DBL_MANT_DIG - 1;
	const uint64_t mant_mask = (UINT64_C(1) << mant_bits) - 1;

	union {
		double f;
		uint64_t i;
	} u = { .f = fabs(val) };

	int exp = ((int)(u.i >> mant_bits)) - exp_bias;

	/* If value is less than one, return zero with appropriate sign. */
	if (exp < 0)
		return copysign(0.0, val);

	if (exp >= mant_bits)
		return val;

	/* Truncate irrelevant fraction bits */
	u.i &= ~(mant_mask >> exp);
	return copysign(u.f, val);
}

/** @}
 */
