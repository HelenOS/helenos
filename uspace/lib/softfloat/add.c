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
/** @file Addition functions.
 */

#include "add.h"
#include "comparison.h"
#include "common.h"
#include "sub.h"

/** Add two single-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 * @return Result of addition.
 */
float32 add_float32(float32 a, float32 b)
{
	int expdiff;
	uint32_t exp1, exp2, frac1, frac2;

	expdiff = a.parts.exp - b.parts.exp;
	if (expdiff < 0) {
		if (is_float32_nan(b)) {
			/* TODO: fix SigNaN */
			if (is_float32_signan(b)) {
			}

			return b;
		}

		if (b.parts.exp == FLOAT32_MAX_EXPONENT) {
			return b;
		}

		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if ((is_float32_nan(a)) || (is_float32_nan(b))) {
			/* TODO: fix SigNaN */
			if (is_float32_signan(a) || is_float32_signan(b)) {
			}
			return (is_float32_nan(a) ? a : b);
		}

		if (a.parts.exp == FLOAT32_MAX_EXPONENT) {
			return a;
		}

		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		frac1 += frac2;
		if (frac1 & FLOAT32_HIDDEN_BIT_MASK) {
			/* result is not denormalized */
			a.parts.exp = 1;
		}
		a.parts.fraction = frac1;
		return a;
	}

	frac1 |= FLOAT32_HIDDEN_BIT_MASK; /* add hidden bit */

	if (exp2 == 0) {
		/* second operand is denormalized */
		--expdiff;
	} else {
		/* add hidden bit to second operand */
		frac2 |= FLOAT32_HIDDEN_BIT_MASK;
	}

	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;

	if (expdiff < (FLOAT32_FRACTION_SIZE + 2)) {
		frac2 >>= expdiff;
		frac1 += frac2;
	} else {
		a.parts.exp = exp1;
		a.parts.fraction = (frac1 >> 6) & (~(FLOAT32_HIDDEN_BIT_MASK));
		return a;
	}

	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	}

	/* rounding - if first bit after fraction is set then round up */
	frac1 += (0x1 << 5);

	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		/* rounding overflow */
		++exp1;
		frac1 >>= 1;
	}

	if ((exp1 == FLOAT32_MAX_EXPONENT) || (exp2 > exp1)) {
		/* overflow - set infinity as result */
		a.parts.exp = FLOAT32_MAX_EXPONENT;
		a.parts.fraction = 0;
		return a;
	}

	a.parts.exp = exp1;

	/* Clear hidden bit and shift */
	a.parts.fraction = ((frac1 >> 6) & (~FLOAT32_HIDDEN_BIT_MASK));
	return a;
}

/** Add two double-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 * @return Result of addition.
 */
float64 add_float64(float64 a, float64 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1, frac2;

	expdiff = ((int) a.parts.exp) - b.parts.exp;
	if (expdiff < 0) {
		if (is_float64_nan(b)) {
			/* TODO: fix SigNaN */
			if (is_float64_signan(b)) {
			}

			return b;
		}

		/* b is infinity and a not */
		if (b.parts.exp == FLOAT64_MAX_EXPONENT) {
			return b;
		}

		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (is_float64_nan(a)) {
			/* TODO: fix SigNaN */
			if (is_float64_signan(a) || is_float64_signan(b)) {
			}
			return a;
		}

		/* a is infinity and b not */
		if (a.parts.exp == FLOAT64_MAX_EXPONENT) {
			return a;
		}

		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		frac1 += frac2;
		if (frac1 & FLOAT64_HIDDEN_BIT_MASK) {
			/* result is not denormalized */
			a.parts.exp = 1;
		}
		a.parts.fraction = frac1;
		return a;
	}

	/* add hidden bit - frac1 is sure not denormalized */
	frac1 |= FLOAT64_HIDDEN_BIT_MASK;

	/* second operand ... */
	if (exp2 == 0) {
		/* ... is denormalized */
		--expdiff;
	} else {
		/* is not denormalized */
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	}

	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;

	if (expdiff < (FLOAT64_FRACTION_SIZE + 2)) {
		frac2 >>= expdiff;
		frac1 += frac2;
	} else {
		a.parts.exp = exp1;
		a.parts.fraction = (frac1 >> 6) & (~(FLOAT64_HIDDEN_BIT_MASK));
		return a;
	}

	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	}

	/* rounding - if first bit after fraction is set then round up */
	frac1 += (0x1 << 5);

	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7)) {
		/* rounding overflow */
		++exp1;
		frac1 >>= 1;
	}

	if ((exp1 == FLOAT64_MAX_EXPONENT) || (exp2 > exp1)) {
		/* overflow - set infinity as result */
		a.parts.exp = FLOAT64_MAX_EXPONENT;
		a.parts.fraction = 0;
		return a;
	}

	a.parts.exp = exp1;
	/* Clear hidden bit and shift */
	a.parts.fraction = ((frac1 >> 6) & (~FLOAT64_HIDDEN_BIT_MASK));
	return a;
}

/** Add two quadruple-precision floats with the same sign.
 *
 * @param a First input operand.
 * @param b Second input operand.
 * @return Result of addition.
 */
float128 add_float128(float128 a, float128 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1_hi, frac1_lo, frac2_hi, frac2_lo, tmp_hi, tmp_lo;

	expdiff = ((int) a.parts.exp) - b.parts.exp;
	if (expdiff < 0) {
		if (is_float128_nan(b)) {
			/* TODO: fix SigNaN */
			if (is_float128_signan(b)) {
			}

			return b;
		}

		/* b is infinity and a not */
		if (b.parts.exp == FLOAT128_MAX_EXPONENT) {
			return b;
		}

		frac1_hi = b.parts.frac_hi;
		frac1_lo = b.parts.frac_lo;
		exp1 = b.parts.exp;
		frac2_hi = a.parts.frac_hi;
		frac2_lo = a.parts.frac_lo;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (is_float128_nan(a)) {
			/* TODO: fix SigNaN */
			if (is_float128_signan(a) || is_float128_signan(b)) {
			}
			return a;
		}

		/* a is infinity and b not */
		if (a.parts.exp == FLOAT128_MAX_EXPONENT) {
			return a;
		}

		frac1_hi = a.parts.frac_hi;
		frac1_lo = a.parts.frac_lo;
		exp1 = a.parts.exp;
		frac2_hi = b.parts.frac_hi;
		frac2_lo = b.parts.frac_lo;
		exp2 = b.parts.exp;
	}

	if (exp1 == 0) {
		/* both are denormalized */
		add128(frac1_hi, frac1_lo, frac2_hi, frac2_lo, &frac1_hi, &frac1_lo);

		and128(frac1_hi, frac1_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hi, &tmp_lo);
		if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
			/* result is not denormalized */
			a.parts.exp = 1;
		}

		a.parts.frac_hi = frac1_hi;
		a.parts.frac_lo = frac1_lo;
		return a;
	}

	/* add hidden bit - frac1 is sure not denormalized */
	or128(frac1_hi, frac1_lo,
	    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &frac1_hi, &frac1_lo);

	/* second operand ... */
	if (exp2 == 0) {
		/* ... is denormalized */
		--expdiff;
	} else {
		/* is not denormalized */
		or128(frac2_hi, frac2_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &frac2_hi, &frac2_lo);
	}

	/* create some space for rounding */
	lshift128(frac1_hi, frac1_lo, 6, &frac1_hi, &frac1_lo);
	lshift128(frac2_hi, frac2_lo, 6, &frac2_hi, &frac2_lo);

	if (expdiff < (FLOAT128_FRACTION_SIZE + 2)) {
		rshift128(frac2_hi, frac2_lo, expdiff, &frac2_hi, &frac2_lo);
		add128(frac1_hi, frac1_lo, frac2_hi, frac2_lo, &frac1_hi, &frac1_lo);
	} else {
		a.parts.exp = exp1;

		rshift128(frac1_hi, frac1_lo, 6, &frac1_hi, &frac1_lo);
		not128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hi, &tmp_lo);
		and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);

		a.parts.frac_hi = tmp_hi;
		a.parts.frac_lo = tmp_lo;
		return a;
	}

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO, 7,
	    &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
		++exp1;
		rshift128(frac1_hi, frac1_lo, 1, &frac1_hi, &frac1_lo);
	}

	/* rounding - if first bit after fraction is set then round up */
	add128(frac1_hi, frac1_lo, 0x0ll, 0x1ll << 5, &frac1_hi, &frac1_lo);

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO, 7,
	    &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
		/* rounding overflow */
		++exp1;
		rshift128(frac1_hi, frac1_lo, 1, &frac1_hi, &frac1_lo);
	}

	if ((exp1 == FLOAT128_MAX_EXPONENT) || (exp2 > exp1)) {
		/* overflow - set infinity as result */
		a.parts.exp = FLOAT64_MAX_EXPONENT;
		a.parts.frac_hi = 0;
		a.parts.frac_lo = 0;
		return a;
	}

	a.parts.exp = exp1;

	/* Clear hidden bit and shift */
	rshift128(frac1_hi, frac1_lo, 6, &frac1_hi, &frac1_lo);
	not128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &tmp_hi, &tmp_lo);
	and128(frac1_hi, frac1_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);

	a.parts.frac_hi = tmp_hi;
	a.parts.frac_lo = tmp_lo;

	return a;
}

#ifdef float32_t

float32_t __addsf3(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		if (ua.data.parts.sign) {
			ua.data.parts.sign = 0;
			res.data = sub_float32(ub.data, ua.data);
		} else {
			ub.data.parts.sign = 0;
			res.data = sub_float32(ua.data, ub.data);
		}
	} else
		res.data = add_float32(ua.data, ub.data);

	return res.val;
}

float32_t __aeabi_fadd(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		if (ua.data.parts.sign) {
			ua.data.parts.sign = 0;
			res.data = sub_float32(ub.data, ua.data);
		} else {
			ub.data.parts.sign = 0;
			res.data = sub_float32(ua.data, ub.data);
		}
	} else
		res.data = add_float32(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float64_t

float64_t __adddf3(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		if (ua.data.parts.sign) {
			ua.data.parts.sign = 0;
			res.data = sub_float64(ub.data, ua.data);
		} else {
			ub.data.parts.sign = 0;
			res.data = sub_float64(ua.data, ub.data);
		}
	} else
		res.data = add_float64(ua.data, ub.data);

	return res.val;
}

float64_t __aeabi_dadd(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		if (ua.data.parts.sign) {
			ua.data.parts.sign = 0;
			res.data = sub_float64(ub.data, ua.data);
		} else {
			ub.data.parts.sign = 0;
			res.data = sub_float64(ua.data, ub.data);
		}
	} else
		res.data = add_float64(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float128_t

float128_t __addtf3(float128_t a, float128_t b)
{
	float128_u ua;
	ua.val = a;

	float128_u ub;
	ub.val = b;

	float128_u res;

	if (ua.data.parts.sign != ub.data.parts.sign) {
		if (ua.data.parts.sign) {
			ua.data.parts.sign = 0;
			res.data = sub_float128(ub.data, ua.data);
		} else {
			ub.data.parts.sign = 0;
			res.data = sub_float128(ua.data, ub.data);
		}
	} else
		res.data = add_float128(ua.data, ub.data);

	return res.val;
}

void _Qp_add(float128_t *c, float128_t *a, float128_t *b)
{
	*c = __addtf3(*a, *b);
}

#endif

/** @}
 */
