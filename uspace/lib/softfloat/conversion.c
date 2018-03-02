/*
 * Copyright (c) 2005 Josef Cejka
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup softfloat
 * @{
 */
/** @file Conversion of precision and conversion between integers and floats.
 */

#include "conversion.h"
#include "comparison.h"
#include "common.h"

float64 float32_to_float64(float32 a)
{
	float64 result;
	uint64_t frac;

	result.parts.sign = a.parts.sign;
	result.parts.fraction = a.parts.fraction;
	result.parts.fraction <<= (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE);

	if ((is_float32_infinity(a)) || (is_float32_nan(a))) {
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		// TODO; check if its correct for SigNaNs
		return result;
	}

	result.parts.exp = a.parts.exp + ((int) FLOAT64_BIAS - FLOAT32_BIAS);
	if (a.parts.exp == 0) {
		/* normalize denormalized numbers */

		if (result.parts.fraction == 0) { /* fix zero */
			result.parts.exp = 0;
			return result;
		}

		frac = result.parts.fraction;

		while (!(frac & FLOAT64_HIDDEN_BIT_MASK)) {
			frac <<= 1;
			--result.parts.exp;
		}

		++result.parts.exp;
		result.parts.fraction = frac;
	}

	return result;
}

float128 float32_to_float128(float32 a)
{
	float128 result;
	uint64_t frac_hi, frac_lo;
	uint64_t tmp_hi, tmp_lo;

	result.parts.sign = a.parts.sign;
	result.parts.frac_hi = 0;
	result.parts.frac_lo = a.parts.fraction;
	lshift128(result.parts.frac_hi, result.parts.frac_lo,
	    (FLOAT128_FRACTION_SIZE - FLOAT32_FRACTION_SIZE),
	    &frac_hi, &frac_lo);
	result.parts.frac_hi = frac_hi;
	result.parts.frac_lo = frac_lo;

	if ((is_float32_infinity(a)) || (is_float32_nan(a))) {
		result.parts.exp = FLOAT128_MAX_EXPONENT;
		// TODO; check if its correct for SigNaNs
		return result;
	}

	result.parts.exp = a.parts.exp + ((int) FLOAT128_BIAS - FLOAT32_BIAS);
	if (a.parts.exp == 0) {
		/* normalize denormalized numbers */

		if (eq128(result.parts.frac_hi,
		    result.parts.frac_lo, 0x0ll, 0x0ll)) { /* fix zero */
			result.parts.exp = 0;
			return result;
		}

		frac_hi = result.parts.frac_hi;
		frac_lo = result.parts.frac_lo;

		and128(frac_hi, frac_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hi, &tmp_lo);
		while (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
			lshift128(frac_hi, frac_lo, 1, &frac_hi, &frac_lo);
			--result.parts.exp;
		}

		++result.parts.exp;
		result.parts.frac_hi = frac_hi;
		result.parts.frac_lo = frac_lo;
	}

	return result;
}

float128 float64_to_float128(float64 a)
{
	float128 result;
	uint64_t frac_hi, frac_lo;
	uint64_t tmp_hi, tmp_lo;

	result.parts.sign = a.parts.sign;
	result.parts.frac_hi = 0;
	result.parts.frac_lo = a.parts.fraction;
	lshift128(result.parts.frac_hi, result.parts.frac_lo,
	    (FLOAT128_FRACTION_SIZE - FLOAT64_FRACTION_SIZE),
	    &frac_hi, &frac_lo);
	result.parts.frac_hi = frac_hi;
	result.parts.frac_lo = frac_lo;

	if ((is_float64_infinity(a)) || (is_float64_nan(a))) {
		result.parts.exp = FLOAT128_MAX_EXPONENT;
		// TODO; check if its correct for SigNaNs
		return result;
	}

	result.parts.exp = a.parts.exp + ((int) FLOAT128_BIAS - FLOAT64_BIAS);
	if (a.parts.exp == 0) {
		/* normalize denormalized numbers */

		if (eq128(result.parts.frac_hi,
		    result.parts.frac_lo, 0x0ll, 0x0ll)) { /* fix zero */
			result.parts.exp = 0;
			return result;
		}

		frac_hi = result.parts.frac_hi;
		frac_lo = result.parts.frac_lo;

		and128(frac_hi, frac_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hi, &tmp_lo);
		while (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
			lshift128(frac_hi, frac_lo, 1, &frac_hi, &frac_lo);
			--result.parts.exp;
		}

		++result.parts.exp;
		result.parts.frac_hi = frac_hi;
		result.parts.frac_lo = frac_lo;
	}

	return result;
}

float32 float64_to_float32(float64 a)
{
	float32 result;
	int32_t exp;
	uint64_t frac;

	result.parts.sign = a.parts.sign;

	if (is_float64_nan(a)) {
		result.parts.exp = FLOAT32_MAX_EXPONENT;

		if (is_float64_signan(a)) {
			/* set first bit of fraction nonzero */
			result.parts.fraction = FLOAT32_HIDDEN_BIT_MASK >> 1;
			return result;
		}

		/* fraction nonzero but its first bit is zero */
		result.parts.fraction = 0x1;
		return result;
	}

	if (is_float64_infinity(a)) {
		result.parts.fraction = 0;
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		return result;
	}

	exp = (int) a.parts.exp - FLOAT64_BIAS + FLOAT32_BIAS;

	if (exp >= FLOAT32_MAX_EXPONENT) {
		/* FIXME: overflow */
		result.parts.fraction = 0;
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		return result;
	} else if (exp <= 0) {
		/* underflow or denormalized */

		result.parts.exp = 0;

		exp *= -1;
		if (exp > FLOAT32_FRACTION_SIZE) {
			/* FIXME: underflow */
			result.parts.fraction = 0;
			return result;
		}

		/* denormalized */

		frac = a.parts.fraction;
		frac |= FLOAT64_HIDDEN_BIT_MASK; /* denormalize and set hidden bit */

		frac >>= (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE + 1);

		while (exp > 0) {
			--exp;
			frac >>= 1;
		}
		result.parts.fraction = frac;

		return result;
	}

	result.parts.exp = exp;
	result.parts.fraction =
	    a.parts.fraction >> (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE);
	return result;
}

float32 float128_to_float32(float128 a)
{
	float32 result;
	int32_t exp;
	uint64_t frac_hi, frac_lo;

	result.parts.sign = a.parts.sign;

	if (is_float128_nan(a)) {
		result.parts.exp = FLOAT32_MAX_EXPONENT;

		if (is_float128_signan(a)) {
			/* set first bit of fraction nonzero */
			result.parts.fraction = FLOAT32_HIDDEN_BIT_MASK >> 1;
			return result;
		}

		/* fraction nonzero but its first bit is zero */
		result.parts.fraction = 0x1;
		return result;
	}

	if (is_float128_infinity(a)) {
		result.parts.fraction = 0;
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		return result;
	}

	exp = (int) a.parts.exp - FLOAT128_BIAS + FLOAT32_BIAS;

	if (exp >= FLOAT32_MAX_EXPONENT) {
		/* FIXME: overflow */
		result.parts.fraction = 0;
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		return result;
	} else if (exp <= 0) {
		/* underflow or denormalized */

		result.parts.exp = 0;

		exp *= -1;
		if (exp > FLOAT32_FRACTION_SIZE) {
			/* FIXME: underflow */
			result.parts.fraction = 0;
			return result;
		}

		/* denormalized */

		frac_hi = a.parts.frac_hi;
		frac_lo = a.parts.frac_lo;

		/* denormalize and set hidden bit */
		frac_hi |= FLOAT128_HIDDEN_BIT_MASK_HI;

		rshift128(frac_hi, frac_lo,
		    (FLOAT128_FRACTION_SIZE - FLOAT32_FRACTION_SIZE + 1),
		    &frac_hi, &frac_lo);

		while (exp > 0) {
			--exp;
			rshift128(frac_hi, frac_lo, 1, &frac_hi, &frac_lo);
		}
		result.parts.fraction = frac_lo;

		return result;
	}

	result.parts.exp = exp;
	frac_hi = a.parts.frac_hi;
	frac_lo = a.parts.frac_lo;
	rshift128(frac_hi, frac_lo,
	    (FLOAT128_FRACTION_SIZE - FLOAT32_FRACTION_SIZE + 1),
	    &frac_hi, &frac_lo);
	result.parts.fraction = frac_lo;
	return result;
}

float64 float128_to_float64(float128 a)
{
	float64 result;
	int32_t exp;
	uint64_t frac_hi, frac_lo;

	result.parts.sign = a.parts.sign;

	if (is_float128_nan(a)) {
		result.parts.exp = FLOAT64_MAX_EXPONENT;

		if (is_float128_signan(a)) {
			/* set first bit of fraction nonzero */
			result.parts.fraction = FLOAT64_HIDDEN_BIT_MASK >> 1;
			return result;
		}

		/* fraction nonzero but its first bit is zero */
		result.parts.fraction = 0x1;
		return result;
	}

	if (is_float128_infinity(a)) {
		result.parts.fraction = 0;
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		return result;
	}

	exp = (int) a.parts.exp - FLOAT128_BIAS + FLOAT64_BIAS;

	if (exp >= FLOAT64_MAX_EXPONENT) {
		/* FIXME: overflow */
		result.parts.fraction = 0;
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		return result;
	} else if (exp <= 0) {
		/* underflow or denormalized */

		result.parts.exp = 0;

		exp *= -1;
		if (exp > FLOAT64_FRACTION_SIZE) {
			/* FIXME: underflow */
			result.parts.fraction = 0;
			return result;
		}

		/* denormalized */

		frac_hi = a.parts.frac_hi;
		frac_lo = a.parts.frac_lo;

		/* denormalize and set hidden bit */
		frac_hi |= FLOAT128_HIDDEN_BIT_MASK_HI;

		rshift128(frac_hi, frac_lo,
		    (FLOAT128_FRACTION_SIZE - FLOAT64_FRACTION_SIZE + 1),
		    &frac_hi, &frac_lo);

		while (exp > 0) {
			--exp;
			rshift128(frac_hi, frac_lo, 1, &frac_hi, &frac_lo);
		}
		result.parts.fraction = frac_lo;

		return result;
	}

	result.parts.exp = exp;
	frac_hi = a.parts.frac_hi;
	frac_lo = a.parts.frac_lo;
	rshift128(frac_hi, frac_lo,
	    (FLOAT128_FRACTION_SIZE - FLOAT64_FRACTION_SIZE + 1),
	    &frac_hi, &frac_lo);
	result.parts.fraction = frac_lo;
	return result;
}

/** Helper procedure for converting float32 to uint32.
 *
 * @param a Floating point number in normalized form
 *     (NaNs or Inf are not checked).
 * @return Converted unsigned integer.
 */
static uint32_t _float32_to_uint32_helper(float32 a)
{
	uint32_t frac;

	if (a.parts.exp < FLOAT32_BIAS) {
		/* TODO: rounding */
		return 0;
	}

	frac = a.parts.fraction;

	frac |= FLOAT32_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 32 - FLOAT32_FRACTION_SIZE - 1;

	frac >>= 32 - (a.parts.exp - FLOAT32_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}

	return frac;
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint32_t float32_to_uint32(float32 a)
{
	if (is_float32_nan(a))
		return UINT32_MAX;

	if (is_float32_infinity(a) || (a.parts.exp >= (32 + FLOAT32_BIAS))) {
		if (a.parts.sign)
			return UINT32_MIN;

		return UINT32_MAX;
	}

	return _float32_to_uint32_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int32_t float32_to_int32(float32 a)
{
	if (is_float32_nan(a))
		return INT32_MAX;

	if (is_float32_infinity(a) || (a.parts.exp >= (32 + FLOAT32_BIAS))) {
		if (a.parts.sign)
			return INT32_MIN;

		return INT32_MAX;
	}

	return _float32_to_uint32_helper(a);
}

/** Helper procedure for converting float32 to uint64.
 *
 * @param a Floating point number in normalized form
 *     (NaNs or Inf are not checked).
 * @return Converted unsigned integer.
 */
static uint64_t _float32_to_uint64_helper(float32 a)
{
	uint64_t frac;

	if (a.parts.exp < FLOAT32_BIAS) {
		// TODO: rounding
		return 0;
	}

	frac = a.parts.fraction;

	frac |= FLOAT32_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 64 - FLOAT32_FRACTION_SIZE - 1;

	frac >>= 64 - (a.parts.exp - FLOAT32_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}

	return frac;
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint64_t float32_to_uint64(float32 a)
{
	if (is_float32_nan(a))
		return UINT64_MAX;

	if (is_float32_infinity(a) || (a.parts.exp >= (64 + FLOAT32_BIAS))) {
		if (a.parts.sign)
			return UINT64_MIN;

		return UINT64_MAX;
	}

	return _float32_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int64_t float32_to_int64(float32 a)
{
	if (is_float32_nan(a))
		return INT64_MAX;

	if (is_float32_infinity(a) || (a.parts.exp >= (64 + FLOAT32_BIAS))) {
		if (a.parts.sign)
			return INT64_MIN;

		return INT64_MAX;
	}

	return _float32_to_uint64_helper(a);
}

/** Helper procedure for converting float64 to uint64.
 *
 * @param a Floating point number in normalized form
 *     (NaNs or Inf are not checked).
 * @return Converted unsigned integer.
 */
static uint64_t _float64_to_uint64_helper(float64 a)
{
	uint64_t frac;

	if (a.parts.exp < FLOAT64_BIAS) {
		// TODO: rounding
		return 0;
	}

	frac = a.parts.fraction;

	frac |= FLOAT64_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 64 - FLOAT64_FRACTION_SIZE - 1;

	frac >>= 64 - (a.parts.exp - FLOAT64_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}

	return frac;
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint32_t float64_to_uint32(float64 a)
{
	if (is_float64_nan(a))
		return UINT32_MAX;

	if (is_float64_infinity(a) || (a.parts.exp >= (32 + FLOAT64_BIAS))) {
		if (a.parts.sign)
			return UINT32_MIN;

		return UINT32_MAX;
	}

	return (uint32_t) _float64_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int32_t float64_to_int32(float64 a)
{
	if (is_float64_nan(a))
		return INT32_MAX;

	if (is_float64_infinity(a) || (a.parts.exp >= (32 + FLOAT64_BIAS))) {
		if (a.parts.sign)
			return INT32_MIN;

		return INT32_MAX;
	}

	return (int32_t) _float64_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint64_t float64_to_uint64(float64 a)
{
	if (is_float64_nan(a))
		return UINT64_MAX;

	if (is_float64_infinity(a) || (a.parts.exp >= (64 + FLOAT64_BIAS))) {
		if (a.parts.sign)
			return UINT64_MIN;

		return UINT64_MAX;
	}

	return _float64_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int64_t float64_to_int64(float64 a)
{
	if (is_float64_nan(a))
		return INT64_MAX;

	if (is_float64_infinity(a) || (a.parts.exp >= (64 + FLOAT64_BIAS))) {
		if (a.parts.sign)
			return INT64_MIN;

		return INT64_MAX;
	}

	return _float64_to_uint64_helper(a);
}

/** Helper procedure for converting float128 to uint64.
 *
 * @param a Floating point number in normalized form
 *     (NaNs or Inf are not checked).
 * @return Converted unsigned integer.
 */
static uint64_t _float128_to_uint64_helper(float128 a)
{
	uint64_t frac_hi, frac_lo;

	if (a.parts.exp < FLOAT128_BIAS) {
		// TODO: rounding
		return 0;
	}

	frac_hi = a.parts.frac_hi;
	frac_lo = a.parts.frac_lo;

	frac_hi |= FLOAT128_HIDDEN_BIT_MASK_HI;
	/* shift fraction to left so hidden bit will be the most significant bit */
	lshift128(frac_hi, frac_lo,
	    (128 - FLOAT128_FRACTION_SIZE - 1), &frac_hi, &frac_lo);

	rshift128(frac_hi, frac_lo,
	    (128 - (a.parts.exp - FLOAT128_BIAS) - 1), &frac_hi, &frac_lo);
	if ((a.parts.sign == 1) && !eq128(frac_hi, frac_lo, 0x0ll, 0x0ll)) {
		not128(frac_hi, frac_lo, &frac_hi, &frac_lo);
		add128(frac_hi, frac_lo, 0x0ll, 0x1ll, &frac_hi, &frac_lo);
	}

	return frac_lo;
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint32_t float128_to_uint32(float128 a)
{
	if (is_float128_nan(a))
		return UINT32_MAX;

	if (is_float128_infinity(a) || (a.parts.exp >= (32 + FLOAT128_BIAS))) {
		if (a.parts.sign)
			return UINT32_MIN;

		return UINT32_MAX;
	}

	return (uint32_t) _float128_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int32_t float128_to_int32(float128 a)
{
	if (is_float128_nan(a))
		return INT32_MAX;

	if (is_float128_infinity(a) || (a.parts.exp >= (32 + FLOAT128_BIAS))) {
		if (a.parts.sign)
			return INT32_MIN;

		return INT32_MAX;
	}

	return (int32_t) _float128_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
uint64_t float128_to_uint64(float128 a)
{
	if (is_float128_nan(a))
		return UINT64_MAX;

	if (is_float128_infinity(a) || (a.parts.exp >= (64 + FLOAT128_BIAS))) {
		if (a.parts.sign)
			return UINT64_MIN;

		return UINT64_MAX;
	}

	return _float128_to_uint64_helper(a);
}

/*
 * FIXME: Im not sure what to return if overflow/underflow happens
 *  - now its the biggest or the smallest int
 */
int64_t float128_to_int64(float128 a)
{
	if (is_float128_nan(a))
		return INT64_MAX;

	if (is_float128_infinity(a) || (a.parts.exp >= (64 + FLOAT128_BIAS))) {
		if (a.parts.sign)
			return INT64_MIN;

		return INT64_MAX;
	}

	return _float128_to_uint64_helper(a);
}

float32 uint32_to_float32(uint32_t i)
{
	int counter;
	int32_t exp;
	float32 result;

	result.parts.sign = 0;
	result.parts.fraction = 0;

	counter = count_zeroes32(i);

	exp = FLOAT32_BIAS + 32 - counter - 1;

	if (counter == 32) {
		result.bin = 0;
		return result;
	}

	if (counter > 0) {
		i <<= counter - 1;
	} else {
		i >>= 1;
	}

	round_float32(&exp, &i);

	result.parts.fraction = i >> (32 - FLOAT32_FRACTION_SIZE - 2);
	result.parts.exp = exp;

	return result;
}

float32 int32_to_float32(int32_t i)
{
	float32 result;

	if (i < 0)
		result = uint32_to_float32((uint32_t) (-i));
	else
		result = uint32_to_float32((uint32_t) i);

	result.parts.sign = i < 0;

	return result;
}

float32 uint64_to_float32(uint64_t i)
{
	int counter;
	int32_t exp;
	uint32_t j;
	float32 result;

	result.parts.sign = 0;
	result.parts.fraction = 0;

	counter = count_zeroes64(i);

	exp = FLOAT32_BIAS + 64 - counter - 1;

	if (counter == 64) {
		result.bin = 0;
		return result;
	}

	/* Shift all to the first 31 bits (31st will be hidden 1) */
	if (counter > 33) {
		i <<= counter - 1 - 32;
	} else {
		i >>= 1 + 32 - counter;
	}

	j = (uint32_t) i;
	round_float32(&exp, &j);

	result.parts.fraction = j >> (32 - FLOAT32_FRACTION_SIZE - 2);
	result.parts.exp = exp;
	return result;
}

float32 int64_to_float32(int64_t i)
{
	float32 result;

	if (i < 0)
		result = uint64_to_float32((uint64_t) (-i));
	else
		result = uint64_to_float32((uint64_t) i);

	result.parts.sign = i < 0;

	return result;
}

float64 uint32_to_float64(uint32_t i)
{
	int counter;
	int32_t exp;
	float64 result;
	uint64_t frac;

	result.parts.sign = 0;
	result.parts.fraction = 0;

	counter = count_zeroes32(i);

	exp = FLOAT64_BIAS + 32 - counter - 1;

	if (counter == 32) {
		result.bin = 0;
		return result;
	}

	frac = i;
	frac <<= counter + 32 - 1;

	round_float64(&exp, &frac);

	result.parts.fraction = frac >> (64 - FLOAT64_FRACTION_SIZE - 2);
	result.parts.exp = exp;

	return result;
}

float64 int32_to_float64(int32_t i)
{
	float64 result;

	if (i < 0)
		result = uint32_to_float64((uint32_t) (-i));
	else
		result = uint32_to_float64((uint32_t) i);

	result.parts.sign = i < 0;

	return result;
}

float64 uint64_to_float64(uint64_t i)
{
	int counter;
	int32_t exp;
	float64 result;

	result.parts.sign = 0;
	result.parts.fraction = 0;

	counter = count_zeroes64(i);

	exp = FLOAT64_BIAS + 64 - counter - 1;

	if (counter == 64) {
		result.bin = 0;
		return result;
	}

	if (counter > 0) {
		i <<= counter - 1;
	} else {
		i >>= 1;
	}

	round_float64(&exp, &i);

	result.parts.fraction = i >> (64 - FLOAT64_FRACTION_SIZE - 2);
	result.parts.exp = exp;
	return result;
}

float64 int64_to_float64(int64_t i)
{
	float64 result;

	if (i < 0)
		result = uint64_to_float64((uint64_t) (-i));
	else
		result = uint64_to_float64((uint64_t) i);

	result.parts.sign = i < 0;

	return result;
}

float128 uint32_to_float128(uint32_t i)
{
	int counter;
	int32_t exp;
	float128 result;
	uint64_t frac_hi, frac_lo;

	result.parts.sign = 0;
	result.parts.frac_hi = 0;
	result.parts.frac_lo = 0;

	counter = count_zeroes32(i);

	exp = FLOAT128_BIAS + 32 - counter - 1;

	if (counter == 32) {
		result.bin.hi = 0;
		result.bin.lo = 0;
		return result;
	}

	frac_hi = 0;
	frac_lo = i;
	lshift128(frac_hi, frac_lo, (counter + 96 - 1), &frac_hi, &frac_lo);

	round_float128(&exp, &frac_hi, &frac_lo);

	rshift128(frac_hi, frac_lo,
	    (128 - FLOAT128_FRACTION_SIZE - 2), &frac_hi, &frac_lo);
	result.parts.frac_hi = frac_hi;
	result.parts.frac_lo = frac_lo;
	result.parts.exp = exp;

	return result;
}

float128 int32_to_float128(int32_t i)
{
	float128 result;

	if (i < 0)
		result = uint32_to_float128((uint32_t) (-i));
	else
		result = uint32_to_float128((uint32_t) i);

	result.parts.sign = i < 0;

	return result;
}


float128 uint64_to_float128(uint64_t i)
{
	int counter;
	int32_t exp;
	float128 result;
	uint64_t frac_hi, frac_lo;

	result.parts.sign = 0;
	result.parts.frac_hi = 0;
	result.parts.frac_lo = 0;

	counter = count_zeroes64(i);

	exp = FLOAT128_BIAS + 64 - counter - 1;

	if (counter == 64) {
		result.bin.hi = 0;
		result.bin.lo = 0;
		return result;
	}

	frac_hi = 0;
	frac_lo = i;
	lshift128(frac_hi, frac_lo, (counter + 64 - 1), &frac_hi, &frac_lo);

	round_float128(&exp, &frac_hi, &frac_lo);

	rshift128(frac_hi, frac_lo,
	    (128 - FLOAT128_FRACTION_SIZE - 2), &frac_hi, &frac_lo);
	result.parts.frac_hi = frac_hi;
	result.parts.frac_lo = frac_lo;
	result.parts.exp = exp;

	return result;
}

float128 int64_to_float128(int64_t i)
{
	float128 result;

	if (i < 0)
		result = uint64_to_float128((uint64_t) (-i));
	else
		result = uint64_to_float128((uint64_t) i);

	result.parts.sign = i < 0;

	return result;
}

#ifdef float32_t

float32_t __floatsisf(int32_t i)
{
	float32_u res;
	res.data = int32_to_float32(i);

	return res.val;
}

float32_t __floatdisf(int64_t i)
{
	float32_u res;
	res.data = int64_to_float32(i);

	return res.val;
}

float32_t __floatunsisf(uint32_t i)
{
	float32_u res;
	res.data = uint32_to_float32(i);

	return res.val;
}

float32_t __floatundisf(uint64_t i)
{
	float32_u res;
	res.data = uint64_to_float32(i);

	return res.val;
}

int32_t __fixsfsi(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_int32(ua.data);
}

int64_t __fixsfdi(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_int64(ua.data);
}

uint32_t __fixunssfsi(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_uint32(ua.data);
}

uint64_t __fixunssfdi(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_uint64(ua.data);
}

int32_t __aeabi_f2iz(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_int32(ua.data);
}

uint32_t __aeabi_f2uiz(float32_t a)
{
	float32_u ua;
	ua.val = a;

	return float32_to_uint32(ua.data);
}

float32_t __aeabi_i2f(int32_t i)
{
	float32_u res;
	res.data = int32_to_float32(i);

	return res.val;
}

float32_t __aeabi_l2f(int64_t i)
{
	float32_u res;
	res.data = int64_to_float32(i);

	return res.val;
}

float32_t __aeabi_ui2f(uint32_t i)
{
	float32_u res;
	res.data = uint32_to_float32(i);

	return res.val;
}

float32_t __aeabi_ul2f(uint64_t i)
{
	float32_u res;
	res.data = uint64_to_float32(i);

	return res.val;
}

#endif

#ifdef float64_t

float64_t __floatsidf(int32_t i)
{
	float64_u res;
	res.data = int32_to_float64(i);

	return res.val;
}

float64_t __floatdidf(int64_t i)
{
	float64_u res;
	res.data = int64_to_float64(i);

	return res.val;
}

float64_t __floatunsidf(uint32_t i)
{
	float64_u res;
	res.data = uint32_to_float64(i);

	return res.val;
}

float64_t __floatundidf(uint64_t i)
{
	float64_u res;
	res.data = uint64_to_float64(i);

	return res.val;
}

uint32_t __fixunsdfsi(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_uint32(ua.data);
}

uint64_t __fixunsdfdi(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_uint64(ua.data);
}

int32_t __fixdfsi(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_int32(ua.data);
}

int64_t __fixdfdi(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_int64(ua.data);
}

float64_t __aeabi_i2d(int32_t i)
{
	float64_u res;
	res.data = int32_to_float64(i);

	return res.val;
}

float64_t __aeabi_ui2d(uint32_t i)
{
	float64_u res;
	res.data = uint32_to_float64(i);

	return res.val;
}

float64_t __aeabi_l2d(int64_t i)
{
	float64_u res;
	res.data = int64_to_float64(i);

	return res.val;
}

int32_t __aeabi_d2iz(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_int32(ua.data);
}

int64_t __aeabi_d2lz(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_int64(ua.data);
}

uint32_t __aeabi_d2uiz(float64_t a)
{
	float64_u ua;
	ua.val = a;

	return float64_to_uint32(ua.data);
}

#endif

#ifdef float128_t

float128_t __floatsitf(int32_t i)
{
	float128_u res;
	res.data = int32_to_float128(i);

	return res.val;
}

float128_t __floatditf(int64_t i)
{
	float128_u res;
	res.data = int64_to_float128(i);

	return res.val;
}

float128_t __floatunsitf(uint32_t i)
{
	float128_u res;
	res.data = uint32_to_float128(i);

	return res.val;
}

float128_t __floatunditf(uint64_t i)
{
	float128_u res;
	res.data = uint64_to_float128(i);

	return res.val;
}

int32_t __fixtfsi(float128_t a)
{
	float128_u ua;
	ua.val = a;

	return float128_to_int32(ua.data);
}

int64_t __fixtfdi(float128_t a)
{
	float128_u ua;
	ua.val = a;

	return float128_to_uint64(ua.data);
}

uint32_t __fixunstfsi(float128_t a)
{
	float128_u ua;
	ua.val = a;

	return float128_to_uint32(ua.data);
}

uint64_t __fixunstfdi(float128_t a)
{
	float128_u ua;
	ua.val = a;

	return float128_to_uint64(ua.data);
}

int32_t _Qp_qtoi(float128_t *a)
{
	return __fixtfsi(*a);
}

int64_t _Qp_qtox(float128_t *a)
{
	return __fixunstfdi(*a);
}

uint32_t _Qp_qtoui(float128_t *a)
{
	return __fixunstfsi(*a);
}

uint64_t _Qp_qtoux(float128_t *a)
{
	return __fixunstfdi(*a);
}

void _Qp_itoq(float128_t *c, int32_t a)
{
	*c = __floatsitf(a);
}

void _Qp_xtoq(float128_t *c, int64_t a)
{
	*c = __floatditf(a);
}

void _Qp_uitoq(float128_t *c, uint32_t a)
{
	*c = __floatunsitf(a);
}

void _Qp_uxtoq(float128_t *c, uint64_t a)
{
	*c = __floatunditf(a);
}

#endif

#if (defined(float32_t) && defined(float64_t))

float32_t __truncdfsf2(float64_t a)
{
	float64_u ua;
	ua.val = a;

	float32_u res;
	res.data = float64_to_float32(ua.data);

	return res.val;
}

float64_t __extendsfdf2(float32_t a)
{
	float32_u ua;
	ua.val = a;

	float64_u res;
	res.data = float32_to_float64(ua.data);

	return res.val;
}

float64_t __aeabi_f2d(float32_t a)
{
	float32_u ua;
	ua.val = a;

	float64_u res;
	res.data = float32_to_float64(ua.data);

	return res.val;
}

float32_t __aeabi_d2f(float64_t a)
{
	float64_u ua;
	ua.val = a;

	float32_u res;
	res.data = float64_to_float32(ua.data);

	return res.val;
}

#endif

#if (defined(float32_t) && defined(float128_t))

float32_t __trunctfsf2(float128_t a)
{
	float128_u ua;
	ua.val = a;

	float32_u res;
	res.data = float128_to_float32(ua.data);

	return res.val;
}

float128_t __extendsftf2(float32_t a)
{
	float32_u ua;
	ua.val = a;

	float128_u res;
	res.data = float32_to_float128(ua.data);

	return res.val;
}

void _Qp_stoq(float128_t *c, float32_t a)
{
	*c = __extendsftf2(a);
}

float32_t _Qp_qtos(float128_t *a)
{
	return __trunctfsf2(*a);
}

#endif

#if (defined(float64_t) && defined(float128_t))

float64_t __trunctfdf2(float128_t a)
{
	float128_u ua;
	ua.val = a;

	float64_u res;
	res.data = float128_to_float64(ua.data);

	return res.val;
}

float128_t __extenddftf2(float64_t a)
{
	float64_u ua;
	ua.val = a;

	float128_u res;
	res.data = float64_to_float128(ua.data);

	return res.val;
}

void _Qp_dtoq(float128_t *c, float64_t a)
{
	*c = __extenddftf2(a);
}

float64_t _Qp_qtod(float128_t *a)
{
	return __trunctfdf2(*a);
}

#endif

/** @}
 */
