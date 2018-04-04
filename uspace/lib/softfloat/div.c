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
/** @file Division functions.
 */

#include "add.h"
#include "div.h"
#include "comparison.h"
#include "mul.h"
#include "common.h"

/** Divide two single-precision floats.
 *
 * @param a Nominator.
 * @param b Denominator.
 *
 * @return Result of division.
 *
 */
float32 div_float32(float32 a, float32 b)
{
	float32 result;
	int32_t aexp, bexp, cexp;
	uint64_t afrac, bfrac, cfrac;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float32_nan(a)) {
		if (is_float32_signan(a)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return a;
	}

	if (is_float32_nan(b)) {
		if (is_float32_signan(b)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return b;
	}

	if (is_float32_infinity(a)) {
		if (is_float32_infinity(b)) {
			/*FIXME: inf / inf */
			result.bin = FLOAT32_NAN;
			return result;
		}
		/* inf / num */
		result.parts.exp = a.parts.exp;
		result.parts.fraction = a.parts.fraction;
		return result;
	}

	if (is_float32_infinity(b)) {
		if (is_float32_zero(a)) {
			/* FIXME 0 / inf */
			result.parts.exp = 0;
			result.parts.fraction = 0;
			return result;
		}
		/* FIXME: num / inf*/
		result.parts.exp = 0;
		result.parts.fraction = 0;
		return result;
	}

	if (is_float32_zero(b)) {
		if (is_float32_zero(a)) {
			/*FIXME: 0 / 0*/
			result.bin = FLOAT32_NAN;
			return result;
		}
		/* FIXME: division by zero */
		result.parts.exp = 0;
		result.parts.fraction = 0;
		return result;
	}

	afrac = a.parts.fraction;
	aexp = a.parts.exp;
	bfrac = b.parts.fraction;
	bexp = b.parts.exp;

	/* denormalized numbers */
	if (aexp == 0) {
		if (afrac == 0) {
			result.parts.exp = 0;
			result.parts.fraction = 0;
			return result;
		}

		/* normalize it*/
		afrac <<= 1;
		/* afrac is nonzero => it must stop */
		while (!(afrac & FLOAT32_HIDDEN_BIT_MASK)) {
			afrac <<= 1;
			aexp--;
		}
	}

	if (bexp == 0) {
		bfrac <<= 1;
		/* bfrac is nonzero => it must stop */
		while (!(bfrac & FLOAT32_HIDDEN_BIT_MASK)) {
			bfrac <<= 1;
			bexp--;
		}
	}

	afrac = (afrac | FLOAT32_HIDDEN_BIT_MASK) << (32 - FLOAT32_FRACTION_SIZE - 1);
	bfrac = (bfrac | FLOAT32_HIDDEN_BIT_MASK) << (32 - FLOAT32_FRACTION_SIZE);

	if (bfrac <= (afrac << 1)) {
		afrac >>= 1;
		aexp++;
	}

	cexp = aexp - bexp + FLOAT32_BIAS - 2;

	cfrac = (afrac << 32) / bfrac;
	if ((cfrac & 0x3F) == 0) {
		cfrac |= (bfrac * cfrac != afrac << 32);
	}

	/* pack and round */

	/* find first nonzero digit and shift result and detect possibly underflow */
	while ((cexp > 0) && (cfrac) && (!(cfrac & (FLOAT32_HIDDEN_BIT_MASK << 7)))) {
		cexp--;
		cfrac <<= 1;
		/* TODO: fix underflow */
	}

	cfrac += (0x1 << 6); /* FIXME: 7 is not sure*/

	if (cfrac & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		++cexp;
		cfrac >>= 1;
	}

	/* check overflow */
	if (cexp >= FLOAT32_MAX_EXPONENT) {
		/* FIXME: overflow, return infinity */
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		result.parts.fraction = 0;
		return result;
	}

	if (cexp < 0) {
		/* FIXME: underflow */
		result.parts.exp = 0;
		if ((cexp + FLOAT32_FRACTION_SIZE) < 0) {
			result.parts.fraction = 0;
			return result;
		}
		cfrac >>= 1;
		while (cexp < 0) {
			cexp++;
			cfrac >>= 1;
		}
	} else {
		result.parts.exp = (uint32_t) cexp;
	}

	result.parts.fraction = ((cfrac >> 6) & (~FLOAT32_HIDDEN_BIT_MASK));

	return result;
}

/** Divide two double-precision floats.
 *
 * @param a Nominator.
 * @param b Denominator.
 *
 * @return Result of division.
 *
 */
float64 div_float64(float64 a, float64 b)
{
	float64 result;
	int64_t aexp, bexp, cexp;
	uint64_t afrac, bfrac, cfrac;
	uint64_t remlo, remhi;
	uint64_t tmplo, tmphi;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float64_nan(a)) {
		if (is_float64_signan(b)) {
			// FIXME: SigNaN
			return b;
		}

		if (is_float64_signan(a)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return a;
	}

	if (is_float64_nan(b)) {
		if (is_float64_signan(b)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return b;
	}

	if (is_float64_infinity(a)) {
		if (is_float64_infinity(b) || is_float64_zero(b)) {
			// FIXME: inf / inf
			result.bin = FLOAT64_NAN;
			return result;
		}
		/* inf / num */
		result.parts.exp = a.parts.exp;
		result.parts.fraction = a.parts.fraction;
		return result;
	}

	if (is_float64_infinity(b)) {
		if (is_float64_zero(a)) {
			/* FIXME 0 / inf */
			result.parts.exp = 0;
			result.parts.fraction = 0;
			return result;
		}
		/* FIXME: num / inf*/
		result.parts.exp = 0;
		result.parts.fraction = 0;
		return result;
	}

	if (is_float64_zero(b)) {
		if (is_float64_zero(a)) {
			/*FIXME: 0 / 0*/
			result.bin = FLOAT64_NAN;
			return result;
		}
		/* FIXME: division by zero */
		result.parts.exp = 0;
		result.parts.fraction = 0;
		return result;
	}

	afrac = a.parts.fraction;
	aexp = a.parts.exp;
	bfrac = b.parts.fraction;
	bexp = b.parts.exp;

	/* denormalized numbers */
	if (aexp == 0) {
		if (afrac == 0) {
			result.parts.exp = 0;
			result.parts.fraction = 0;
			return result;
		}

		/* normalize it*/
		aexp++;
		/* afrac is nonzero => it must stop */
		while (!(afrac & FLOAT64_HIDDEN_BIT_MASK)) {
			afrac <<= 1;
			aexp--;
		}
	}

	if (bexp == 0) {
		bexp++;
		/* bfrac is nonzero => it must stop */
		while (!(bfrac & FLOAT64_HIDDEN_BIT_MASK)) {
			bfrac <<= 1;
			bexp--;
		}
	}

	afrac = (afrac | FLOAT64_HIDDEN_BIT_MASK) << (64 - FLOAT64_FRACTION_SIZE - 2);
	bfrac = (bfrac | FLOAT64_HIDDEN_BIT_MASK) << (64 - FLOAT64_FRACTION_SIZE - 1);

	if (bfrac <= (afrac << 1)) {
		afrac >>= 1;
		aexp++;
	}

	cexp = aexp - bexp + FLOAT64_BIAS - 2;

	cfrac = div128est(afrac, 0x0ll, bfrac);

	if ((cfrac & 0x1FF) <= 2) {
		mul64(bfrac, cfrac, &tmphi, &tmplo);
		sub128(afrac, 0x0ll, tmphi, tmplo, &remhi, &remlo);

		while ((int64_t) remhi < 0) {
			cfrac--;
			add128(remhi, remlo, 0x0ll, bfrac, &remhi, &remlo);
		}
		cfrac |= (remlo != 0);
	}

	/* round and shift */
	result = finish_float64(cexp, cfrac, result.parts.sign);
	return result;
}

/** Divide two quadruple-precision floats.
 *
 * @param a Nominator.
 * @param b Denominator.
 *
 * @return Result of division.
 *
 */
float128 div_float128(float128 a, float128 b)
{
	float128 result;
	int64_t aexp, bexp, cexp;
	uint64_t afrac_hi, afrac_lo, bfrac_hi, bfrac_lo, cfrac_hi, cfrac_lo;
	uint64_t shift_out;
	uint64_t rem_hihi, rem_hilo, rem_lohi, rem_lolo;
	uint64_t tmp_hihi, tmp_hilo, tmp_lohi, tmp_lolo;

	result.parts.sign = a.parts.sign ^ b.parts.sign;

	if (is_float128_nan(a)) {
		if (is_float128_signan(b)) {
			// FIXME: SigNaN
			return b;
		}

		if (is_float128_signan(a)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return a;
	}

	if (is_float128_nan(b)) {
		if (is_float128_signan(b)) {
			// FIXME: SigNaN
		}
		/* NaN */
		return b;
	}

	if (is_float128_infinity(a)) {
		if (is_float128_infinity(b) || is_float128_zero(b)) {
			// FIXME: inf / inf
			result.bin.hi = FLOAT128_NAN_HI;
			result.bin.lo = FLOAT128_NAN_LO;
			return result;
		}
		/* inf / num */
		result.parts.exp = a.parts.exp;
		result.parts.frac_hi = a.parts.frac_hi;
		result.parts.frac_lo = a.parts.frac_lo;
		return result;
	}

	if (is_float128_infinity(b)) {
		if (is_float128_zero(a)) {
			// FIXME 0 / inf
			result.parts.exp = 0;
			result.parts.frac_hi = 0;
			result.parts.frac_lo = 0;
			return result;
		}
		// FIXME: num / inf
		result.parts.exp = 0;
		result.parts.frac_hi = 0;
		result.parts.frac_lo = 0;
		return result;
	}

	if (is_float128_zero(b)) {
		if (is_float128_zero(a)) {
			// FIXME: 0 / 0
			result.bin.hi = FLOAT128_NAN_HI;
			result.bin.lo = FLOAT128_NAN_LO;
			return result;
		}
		// FIXME: division by zero
		result.parts.exp = 0;
		result.parts.frac_hi = 0;
		result.parts.frac_lo = 0;
		return result;
	}

	afrac_hi = a.parts.frac_hi;
	afrac_lo = a.parts.frac_lo;
	aexp = a.parts.exp;
	bfrac_hi = b.parts.frac_hi;
	bfrac_lo = b.parts.frac_lo;
	bexp = b.parts.exp;

	/* denormalized numbers */
	if (aexp == 0) {
		if (eq128(afrac_hi, afrac_lo, 0x0ll, 0x0ll)) {
			result.parts.exp = 0;
			result.parts.frac_hi = 0;
			result.parts.frac_lo = 0;
			return result;
		}

		/* normalize it*/
		aexp++;
		/* afrac is nonzero => it must stop */
		and128(afrac_hi, afrac_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hihi, &tmp_lolo);
		while (!lt128(0x0ll, 0x0ll, tmp_hihi, tmp_lolo)) {
			lshift128(afrac_hi, afrac_lo, 1, &afrac_hi, &afrac_lo);
			aexp--;
		}
	}

	if (bexp == 0) {
		bexp++;
		/* bfrac is nonzero => it must stop */
		and128(bfrac_hi, bfrac_lo,
		    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    &tmp_hihi, &tmp_lolo);
		while (!lt128(0x0ll, 0x0ll, tmp_hihi, tmp_lolo)) {
			lshift128(bfrac_hi, bfrac_lo, 1, &bfrac_hi, &bfrac_lo);
			bexp--;
		}
	}

	or128(afrac_hi, afrac_lo,
	    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &afrac_hi, &afrac_lo);
	lshift128(afrac_hi, afrac_lo,
	    (128 - FLOAT128_FRACTION_SIZE - 1), &afrac_hi, &afrac_lo);
	or128(bfrac_hi, bfrac_lo,
	    FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &bfrac_hi, &bfrac_lo);
	lshift128(bfrac_hi, bfrac_lo,
	    (128 - FLOAT128_FRACTION_SIZE - 1), &bfrac_hi, &bfrac_lo);

	if (le128(bfrac_hi, bfrac_lo, afrac_hi, afrac_lo)) {
		rshift128(afrac_hi, afrac_lo, 1, &afrac_hi, &afrac_lo);
		aexp++;
	}

	cexp = aexp - bexp + FLOAT128_BIAS - 2;

	cfrac_hi = div128est(afrac_hi, afrac_lo, bfrac_hi);

	mul128(bfrac_hi, bfrac_lo, 0x0ll, cfrac_hi,
	    &tmp_lolo /* dummy */, &tmp_hihi, &tmp_hilo, &tmp_lohi);

	/* sub192(afrac_hi, afrac_lo, 0,
	 *     tmp_hihi, tmp_hilo, tmp_lohi
	 *     &rem_hihi, &rem_hilo, &rem_lohi); */
	sub128(afrac_hi, afrac_lo, tmp_hihi, tmp_hilo, &rem_hihi, &rem_hilo);
	if (tmp_lohi > 0) {
		sub128(rem_hihi, rem_hilo, 0x0ll, 0x1ll, &rem_hihi, &rem_hilo);
	}
	rem_lohi = -tmp_lohi;

	while ((int64_t) rem_hihi < 0) {
		--cfrac_hi;
		/* add192(rem_hihi, rem_hilo, rem_lohi,
		 *     0, bfrac_hi, bfrac_lo,
		 *     &rem_hihi, &rem_hilo, &rem_lohi); */
		add128(rem_hilo, rem_lohi, bfrac_hi, bfrac_lo, &rem_hilo, &rem_lohi);
		if (lt128(rem_hilo, rem_lohi, bfrac_hi, bfrac_lo)) {
			++rem_hihi;
		}
	}

	cfrac_lo = div128est(rem_hilo, rem_lohi, bfrac_lo);

	if ((cfrac_lo & 0x3FFF) <= 4) {
		mul128(bfrac_hi, bfrac_lo, 0x0ll, cfrac_lo,
		    &tmp_hihi /* dummy */, &tmp_hilo, &tmp_lohi, &tmp_lolo);

		/* sub192(rem_hilo, rem_lohi, 0,
		 *     tmp_hilo, tmp_lohi, tmp_lolo,
		 *     &rem_hilo, &rem_lohi, &rem_lolo); */
		sub128(rem_hilo, rem_lohi, tmp_hilo, tmp_lohi, &rem_hilo, &rem_lohi);
		if (tmp_lolo > 0) {
			sub128(rem_hilo, rem_lohi, 0x0ll, 0x1ll, &rem_hilo, &rem_lohi);
		}
		rem_lolo = -tmp_lolo;

		while ((int64_t) rem_hilo < 0) {
			--cfrac_lo;
			/* add192(rem_hilo, rem_lohi, rem_lolo,
			 *     0, bfrac_hi, bfrac_lo,
			 *     &rem_hilo, &rem_lohi, &rem_lolo); */
			add128(rem_lohi, rem_lolo, bfrac_hi, bfrac_lo, &rem_lohi, &rem_lolo);
			if (lt128(rem_lohi, rem_lolo, bfrac_hi, bfrac_lo)) {
				++rem_hilo;
			}
		}

		cfrac_lo |= ((rem_hilo | rem_lohi | rem_lolo) != 0);
	}

	shift_out = cfrac_lo << (64 - (128 - FLOAT128_FRACTION_SIZE - 1));
	rshift128(cfrac_hi, cfrac_lo, (128 - FLOAT128_FRACTION_SIZE - 1),
	    &cfrac_hi, &cfrac_lo);

	result = finish_float128(cexp, cfrac_hi, cfrac_lo, result.parts.sign, shift_out);
	return result;
}

#ifdef float32_t

float32_t __divsf3(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;
	res.data = div_float32(ua.data, ub.data);

	return res.val;
}

float32_t __aeabi_fdiv(float32_t a, float32_t b)
{
	float32_u ua;
	ua.val = a;

	float32_u ub;
	ub.val = b;

	float32_u res;
	res.data = div_float32(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float64_t

float64_t __divdf3(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;
	res.data = div_float64(ua.data, ub.data);

	return res.val;
}

float64_t __aeabi_ddiv(float64_t a, float64_t b)
{
	float64_u ua;
	ua.val = a;

	float64_u ub;
	ub.val = b;

	float64_u res;
	res.data = div_float64(ua.data, ub.data);

	return res.val;
}

#endif

#ifdef float128_t

float128_t __divtf3(float128_t a, float128_t b)
{
	float128_u ua;
	ua.val = a;

	float128_u ub;
	ub.val = b;

	float128_u res;
	res.data = div_float128(ua.data, ub.data);

	return res.val;
}

void _Qp_div(float128_t *c, float128_t *a, float128_t *b)
{
	*c = __divtf3(*a, *b);
}

#endif

/** @}
 */
