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
/** @file Substraction functions.
 */

#include "sub.h"
#include "comparison.h"
#include "common.h"
#include "add.h"

/** Subtract two single-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of substraction.
 *
 */
float32 sub_float32(float32 a, float32 b)
{
	int expdiff;
	uint32_t exp1, exp2, frac1, frac2;
	float32 result;

	result.bin = 0;

	expdiff = a.parts.exp - b.parts.exp;
	if ((expdiff < 0 ) || ((expdiff == 0) &&
	    (a.parts.fraction < b.parts.fraction))) {
		if (is_float32_nan(b)) {
			if (is_float32_signan(b)) {
				// TODO: fix SigNaN
			}

			return b;
		}

		if (b.parts.exp == FLOAT32_MAX_EXPONENT) {
			/* num -(+-inf) = -+inf */
			b.parts.sign = !b.parts.sign;
			return b;
		}

		result.parts.sign = !a.parts.sign;

		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (is_float32_nan(a)) {
			if ((is_float32_signan(a)) || (is_float32_signan(b))) {
				// TODO: fix SigNaN
			}

			return a;
		}

		if (a.parts.exp == FLOAT32_MAX_EXPONENT) {
			if (b.parts.exp == FLOAT32_MAX_EXPONENT) {
				/* inf - inf => nan */
				// TODO: fix exception
				result.bin = FLOAT32_NAN;
				return result;
			}

			return a;
		}

		result.parts.sign = a.parts.sign;

		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		result.parts.fraction = frac1 - frac2;
		if (result.parts.fraction > frac1) {
			// TODO: underflow exception
			return result;
		}

		result.parts.exp = 0;
		return result;
	}

	/* add hidden bit */
	frac1 |= FLOAT32_HIDDEN_BIT_MASK;

	if (exp2 == 0) {
		/* denormalized */
		--expdiff;
	} else {
		/* normalized */
		frac2 |= FLOAT32_HIDDEN_BIT_MASK;
	}

	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;

	if (expdiff > FLOAT32_FRACTION_SIZE + 1)
		goto done;

	frac1 = frac1 - (frac2 >> expdiff);

done:
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	while ((exp1 > 0) && (!(frac1 & (FLOAT32_HIDDEN_BIT_MASK << 6 )))) {
		--exp1;
		frac1 <<= 1;
		/* TODO: fix underflow - frac1 == 0 does not necessary means underflow... */
	}

	/* rounding - if first bit after fraction is set then round up */
	frac1 += 0x20;

	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	}

	/* Clear hidden bit and shift */
	result.parts.fraction = ((frac1 >> 6) & (~FLOAT32_HIDDEN_BIT_MASK));
	result.parts.exp = exp1;

	return result;
}

/** Subtract two double-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of substraction.
 *
 */
float64 sub_float64(float64 a, float64 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1, frac2;
	float64 result;

	result.bin = 0;

	expdiff = a.parts.exp - b.parts.exp;
	if ((expdiff < 0 ) ||
	    ((expdiff == 0) && (a.parts.fraction < b.parts.fraction))) {
		if (is_float64_nan(b)) {
			if (is_float64_signan(b)) {
				// TODO: fix SigNaN
			}

			return b;
		}

		if (b.parts.exp == FLOAT64_MAX_EXPONENT) {
			/* num -(+-inf) = -+inf */
			b.parts.sign = !b.parts.sign;
			return b;
		}

		result.parts.sign = !a.parts.sign;

		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (is_float64_nan(a)) {
			if (is_float64_signan(a) || is_float64_signan(b)) {
				// TODO: fix SigNaN
			}

			return a;
		}

		if (a.parts.exp == FLOAT64_MAX_EXPONENT) {
			if (b.parts.exp == FLOAT64_MAX_EXPONENT) {
				/* inf - inf => nan */
				// TODO: fix exception
				result.bin = FLOAT64_NAN;
				return result;
			}

			return a;
		}

		result.parts.sign = a.parts.sign;

		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		result.parts.fraction = frac1 - frac2;
		if (result.parts.fraction > frac1) {
			// TODO: underflow exception
			return result;
		}

		result.parts.exp = 0;
		return result;
	}

	/* add hidden bit */
	frac1 |= FLOAT64_HIDDEN_BIT_MASK;

	if (exp2 == 0) {
		/* denormalized */
		--expdiff;
	} else {
		/* normalized */
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	}

	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;

	if (expdiff > FLOAT64_FRACTION_SIZE + 1)
		goto done;

	frac1 = frac1 - (frac2 >> expdiff);

done:
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	while ((exp1 > 0) && (!(frac1 & (FLOAT64_HIDDEN_BIT_MASK << 6 )))) {
		--exp1;
		frac1 <<= 1;
		/* TODO: fix underflow - frac1 == 0 does not necessary means underflow... */
	}

	/* rounding - if first bit after fraction is set then round up */
	frac1 += 0x20;

	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	}

	/* Clear hidden bit and shift */
	result.parts.fraction = ((frac1 >> 6) & (~FLOAT64_HIDDEN_BIT_MASK));
	result.parts.exp = exp1;

	return result;
}

/** Subtract two quadruple-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 *
 * @return Result of substraction.
 *
 */
float128 sub_float128(float128 a, float128 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1_hi, frac1_lo, frac2_hi, frac2_lo, tmp_hi, tmp_lo;
	float128 result;

	result.bin.hi = 0;
	result.bin.lo = 0;

	expdiff = a.parts.exp - b.parts.exp;
	if ((expdiff < 0 ) || ((expdiff == 0) &&
	    lt128(a.parts.frac_hi, a.parts.frac_lo, b.parts.frac_hi, b.parts.frac_lo))) {
		if (is_float128_nan(b)) {
			if (is_float128_signan(b)) {
				// TODO: fix SigNaN
			}

			return b;
		}

		if (b.parts.exp == FLOAT128_MAX_EXPONENT) {
			/* num -(+-inf) = -+inf */
			b.parts.sign = !b.parts.sign;
			return b;
		}

		result.parts.sign = !a.parts.sign;

		frac1_hi = b.parts.frac_hi;
		frac1_lo = b.parts.frac_lo;
		exp1 = b.parts.exp;
		frac2_hi = a.parts.frac_hi;
		frac2_lo = a.parts.frac_lo;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (is_float128_nan(a)) {
			if (is_float128_signan(a) || is_float128_signan(b)) {
				// TODO: fix SigNaN
			}

			return a;
		}

		if (a.parts.exp == FLOAT128_MAX_EXPONENT) {
			if (b.parts.exp == FLOAT128_MAX_EXPONENT) {
				/* inf - inf => nan */
				// TODO: fix exception
				result.bin.hi = FLOAT128_NAN_HI;
				result.bin.lo = FLOAT128_NAN_LO;
				return result;
			}
			return a;
		}

		result.parts.sign = a.parts.sign;

		frac1_hi = a.parts.frac_hi;
		frac1_lo = a.parts.frac_lo;
		exp1 = a.parts.exp;
		frac2_hi = b.parts.frac_hi;
		frac2_lo = b.parts.frac_lo;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		sub128(frac1_hi, frac1_lo, frac2_hi, frac2_lo, &tmp_hi, &tmp_lo);
		result.parts.frac_hi = tmp_hi;
		result.parts.frac_lo = tmp_lo;
		if (lt128(frac1_hi, frac1_lo, result.parts.frac_hi, result.parts.frac_lo)) {
			// TODO: underflow exception
			return result;
		}

		result.parts.exp = 0;
		return result;
	}

	/* add hidden bit */
	or128(frac1_hi, frac1_lo,
	    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &frac1_hi, &frac1_lo);

	if (exp2 == 0) {
		/* denormalized */
		--expdiff;
	} else {
		/* normalized */
		or128(frac2_hi, frac2_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &frac2_hi, &frac2_lo);
	}

	/* create some space for rounding */
	lshift128(frac1_hi, frac1_lo, 6, &frac1_hi, &frac1_lo);
	lshift128(frac2_hi, frac2_lo, 6, &frac2_hi, &frac2_lo);

	if (expdiff > FLOAT128_FRACTION_SIZE + 1)
		goto done;

	rshift128(frac2_hi, frac2_lo, expdiff, &tmp_hi, &tmp_lo);
	sub128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &frac1_hi, &frac1_lo);

done:
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO, 6,
	    &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	while ((exp1 > 0) && (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo))) {
		--exp1;
		lshift128(frac1_hi, frac1_lo, 1, &frac1_hi, &frac1_lo);
		/* TODO: fix underflow - frac1 == 0 does not necessary means underflow... */

		lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO, 6,
		    &tmp_hi, &tmp_lo);
		and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	}

	/* rounding - if first bit after fraction is set then round up */
	add128(frac1_hi, frac1_lo, 0x0ll, 0x20ll, &frac1_hi, &frac1_lo);

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO, 7,
	   &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
		++exp1;
		rshift128(frac1_hi, frac1_lo, 1, &frac1_hi, &frac1_lo);
	}

	/* Clear hidden bit and shift */
	rshift128(frac1_hi, frac1_lo, 6, &frac1_hi, &frac1_lo);
	not128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	result.parts.frac_hi = tmp_hi;
	result.parts.frac_lo = tmp_lo;

	result.parts.exp = exp1;

	return result;
}

#ifdef float32_t

float32_t __subsf3(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		ub.data.parts.sign = !ub.data.parts.sign;
		res.data = add_float32(ua.data, ub.data);
	} else
		res.data = sub_float32(ua.data, ub.data);

	return res.val;
}

float32_t __aeabi_fsub(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		ub.data.parts.sign = !ub.data.parts.sign;
		res.data = add_float32(ua.data, ub.data);
	} else
		res.data = sub_float32(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float64_t

float64_t __subdf3(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		ub.data.parts.sign = !ub.data.parts.sign;
		res.data = add_float64(ua.data, ub.data);
	} else
		res.data = sub_float64(ua.data, ub.data);

	return res.val;
}

float64_t __aeabi_dsub(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		ub.data.parts.sign = !ub.data.parts.sign;
		res.data = add_float64(ua.data, ub.data);
	} else
		res.data = sub_float64(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float128_t

float128_t __subtf3(float128_t a, float128_t b)
{
	float128_u ua;
	ua.val = a;

	float128_u ub;
	ub.val = b;

	float128_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		ub.data.parts.sign = !ub.data.parts.sign;
		res.data = add_float128(ua.data, ub.data);
	} else
		res.data = sub_float128(ua.data, ub.data);

	return res.val;
}

void _Qp_sub(float128_t *c, float128_t *a, float128_t *b)
{
	*c = __subtf3(*a, *b);
}

#endif

/** @}
 */
