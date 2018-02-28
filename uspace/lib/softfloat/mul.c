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
/** @file Multiplication functions.
 */

#include "mul.h"
#include "comparison.h"
#include "common.h"

/** Multiply two single-precision floats.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of multiplication.
 *
 */
float32 mul_float32(float32 a, float32 b)
{
	float32 result;
	uint64_t frac1, frac2;
	int32_t exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float32_nan(a) || is_float32_nan(b)) {
		/* TODO: fix SigNaNs */
		if (is_float32_signan(a)) {
			result.parts.fraction = a.parts.fraction;
			result.parts.exp = a.parts.exp;
			return result;
		}

		if (is_float32_signan(b)) { /* TODO: fix SigNaN */
			result.parts.fraction = b.parts.fraction;
			result.parts.exp = b.parts.exp;
			return result;
		}

		/* set NaN as result */
		result.bin = FLOAT32_NAN;
		return result;
	}

	if (is_float32_infinity(a)) {
		if (is_float32_zero(b)) {
			/* FIXME: zero * infinity */
			result.bin = FLOAT32_NAN;
			return result;
		}

		result.parts.fraction = a.parts.fraction;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (is_float32_infinity(b)) {
		if (is_float32_zero(a)) {
			/* FIXME: zero * infinity */
			result.bin = FLOAT32_NAN;
			return result;
		}

		result.parts.fraction = b.parts.fraction;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp;
	exp -= FLOAT32_BIAS;

	if (exp >= FLOAT32_MAX_EXPONENT) {
		/* FIXME: overflow */
		/* set infinity as result */
		result.bin = FLOAT32_INF;
		result.parts.sign = a.parts.sign ^ b.parts.sign;
		return result;
	}

	if (exp < 0) {
		/* FIXME: underflow */
		/* return signed zero */
		result.parts.fraction = 0x0;
		result.parts.exp = 0x0;
		return result;
	}

	frac1 = a.parts.fraction;
	if (a.parts.exp > 0) {
		frac1 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	}

	frac2 = b.parts.fraction;

	if (b.parts.exp > 0) {
		frac2 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	}

	frac1 <<= 1; /* one bit space for rounding */

	frac1 = frac1 * frac2;

	/* round and return */
	while ((exp < FLOAT32_MAX_EXPONENT) &&
	    (frac1 >= (1 << (FLOAT32_FRACTION_SIZE + 2)))) {
		/* 23 bits of fraction + one more for hidden bit (all shifted 1 bit left) */
		++exp;
		frac1 >>= 1;
	}

	/* rounding */
	/* ++frac1; FIXME: not works - without it is ok */
	frac1 >>= 1; /* shift off rounding space */

	if ((exp < FLOAT32_MAX_EXPONENT) &&
	    (frac1 >= (1 << (FLOAT32_FRACTION_SIZE + 1)))) {
		++exp;
		frac1 >>= 1;
	}

	if (exp >= FLOAT32_MAX_EXPONENT) {
		/* TODO: fix overflow */
		/* return infinity*/
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		result.parts.fraction = 0x0;
		return result;
	}

	exp -= FLOAT32_FRACTION_SIZE;

	if (exp <= FLOAT32_FRACTION_SIZE) {
		/* denormalized number */
		frac1 >>= 1; /* denormalize */

		while ((frac1 > 0) && (exp < 0)) {
			frac1 >>= 1;
			++exp;
		}

		if (frac1 == 0) {
			/* FIXME : underflow */
			result.parts.exp = 0;
			result.parts.fraction = 0;
			return result;
		}
	}

	result.parts.exp = exp;
	result.parts.fraction = frac1 & ((1 << FLOAT32_FRACTION_SIZE) - 1);

	return result;
}

/** Multiply two double-precision floats.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of multiplication.
 *
 */
float64 mul_float64(float64 a, float64 b)
{
	float64 result;
	uint64_t frac1, frac2;
	int32_t exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float64_nan(a) || is_float64_nan(b)) {
		/* TODO: fix SigNaNs */
		if (is_float64_signan(a)) {
			result.parts.fraction = a.parts.fraction;
			result.parts.exp = a.parts.exp;
			return result;
		}
		if (is_float64_signan(b)) { /* TODO: fix SigNaN */
			result.parts.fraction = b.parts.fraction;
			result.parts.exp = b.parts.exp;
			return result;
		}
		/* set NaN as result */
		result.bin = FLOAT64_NAN;
		return result;
	}

	if (is_float64_infinity(a)) {
		if (is_float64_zero(b)) {
			/* FIXME: zero * infinity */
			result.bin = FLOAT64_NAN;
			return result;
		}
		result.parts.fraction = a.parts.fraction;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (is_float64_infinity(b)) {
		if (is_float64_zero(a)) {
			/* FIXME: zero * infinity */
			result.bin = FLOAT64_NAN;
			return result;
		}
		result.parts.fraction = b.parts.fraction;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp - FLOAT64_BIAS;

	frac1 = a.parts.fraction;

	if (a.parts.exp > 0) {
		frac1 |= FLOAT64_HIDDEN_BIT_MASK;
	} else {
		++exp;
	}

	frac2 = b.parts.fraction;

	if (b.parts.exp > 0) {
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	} else {
		++exp;
	}

	frac1 <<= (64 - FLOAT64_FRACTION_SIZE - 1);
	frac2 <<= (64 - FLOAT64_FRACTION_SIZE - 2);

	mul64(frac1, frac2, &frac1, &frac2);

	frac1 |= (frac2 != 0);
	if (frac1 & (0x1ll << 62)) {
		frac1 <<= 1;
		exp--;
	}

	result = finish_float64(exp, frac1, result.parts.sign);
	return result;
}

/** Multiply two quadruple-precision floats.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of multiplication.
 *
 */
float128 mul_float128(float128 a, float128 b)
{
	float128 result;
	uint64_t frac1_hi, frac1_lo, frac2_hi, frac2_lo, tmp_hi, tmp_lo;
	int32_t exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float128_nan(a) || is_float128_nan(b)) {
		/* TODO: fix SigNaNs */
		if (is_float128_signan(a)) {
			result.parts.frac_hi = a.parts.frac_hi;
			result.parts.frac_lo = a.parts.frac_lo;
			result.parts.exp = a.parts.exp;
			return result;
		}
		if (is_float128_signan(b)) { /* TODO: fix SigNaN */
			result.parts.frac_hi = b.parts.frac_hi;
			result.parts.frac_lo = b.parts.frac_lo;
			result.parts.exp = b.parts.exp;
			return result;
		}
		/* set NaN as result */
		result.bin.hi = FLOAT128_NAN_HI;
		result.bin.lo = FLOAT128_NAN_LO;
		return result;
	}

	if (is_float128_infinity(a)) {
		if (is_float128_zero(b)) {
			/* FIXME: zero * infinity */
			result.bin.hi = FLOAT128_NAN_HI;
			result.bin.lo = FLOAT128_NAN_LO;
			return result;
		}
		result.parts.frac_hi = a.parts.frac_hi;
		result.parts.frac_lo = a.parts.frac_lo;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (is_float128_infinity(b)) {
		if (is_float128_zero(a)) {
			/* FIXME: zero * infinity */
			result.bin.hi = FLOAT128_NAN_HI;
			result.bin.lo = FLOAT128_NAN_LO;
			return result;
		}
		result.parts.frac_hi = b.parts.frac_hi;
		result.parts.frac_lo = b.parts.frac_lo;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp - FLOAT128_BIAS - 1;

	frac1_hi = a.parts.frac_hi;
	frac1_lo = a.parts.frac_lo;

	if (a.parts.exp > 0) {
		or128(frac1_hi, frac1_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &frac1_hi, &frac1_lo);
	} else {
		++exp;
	}

	frac2_hi = b.parts.frac_hi;
	frac2_lo = b.parts.frac_lo;

	if (b.parts.exp > 0) {
		or128(frac2_hi, frac2_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &frac2_hi, &frac2_lo);
	} else {
		++exp;
	}

	lshift128(frac2_hi, frac2_lo,
	    128 - FLOAT128_FRACTION_SIZE, &frac2_hi, &frac2_lo);

	tmp_hi = frac1_hi;
	tmp_lo = frac1_lo;
	mul128(frac1_hi, frac1_lo, frac2_hi, frac2_lo,
	    &frac1_hi, &frac1_lo, &frac2_hi, &frac2_lo);
	add128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &frac1_hi, &frac1_lo);
	frac2_hi |= (frac2_lo != 0x0ll);

	if ((FLOAT128_HIDDEN_BIT_MASK_HI << 1) <= frac1_hi) {
		frac2_hi >>= 1;
		if (frac1_lo & 0x1ll) {
			frac2_hi |= (0x1ull < 64);
		}
		rshift128(frac1_hi, frac1_lo, 1, &frac1_hi, &frac1_lo);
		++exp;
	}

	result = finish_float128(exp, frac1_hi, frac1_lo, result.parts.sign, frac2_hi);
	return result;
}

#ifdef float32_t

float32_t __mulsf3(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;
	res.data = mul_float32(ua.data, ub.data);

	return res.val;
}

float32_t __aeabi_fmul(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;
	res.data = mul_float32(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float64_t

float64_t __muldf3(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;
	res.data = mul_float64(ua.data, ub.data);

	return res.val;
}

float64_t __aeabi_dmul(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;
	res.data = mul_float64(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float128_t

float128_t __multf3(float128_t a, float128_t b)
{
	float128_u ua;
	ua.val = a;

	float128_u ub;
	ub.val = b;

	float128_u res;
	res.data = mul_float128(ua.data, ub.data);

	return res.val;
}

void _Qp_mul(float128_t *c, float128_t *a, float128_t *b)
{
	*c = __multf3(*a, *b);
}

#endif

/** @}
 */
