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
/** @file Common helper operations.
 */

#include "common.h"

/* Table for fast leading zeroes counting. */
char zeroTable[256] = {
	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
 * Take fraction shifted by 10 bits to the left, round it, normalize it
 * and detect exceptions
 *
 * @param cexp Exponent with bias.
 * @param cfrac Fraction shifted 10 bits to the left with added hidden bit.
 * @param sign Resulting sign.
 * @return Finished double-precision float.
 */
float64 finish_float64(int32_t cexp, uint64_t cfrac, char sign)
{
	float64 result;

	result.parts.sign = sign;

	/* find first nonzero digit and shift result and detect possibly underflow */
	while ((cexp > 0) && (cfrac) &&
	    (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1))))) {
		cexp--;
		cfrac <<= 1;
		/* TODO: fix underflow */
	}

	if ((cexp < 0) || (cexp == 0 &&
	    (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1)))))) {
		/* FIXME: underflow */
		result.parts.exp = 0;
		if ((cexp + FLOAT64_FRACTION_SIZE + 1) < 0) { /* +1 is place for rounding */
			result.parts.fraction = 0;
			return result;
		}

		while (cexp < 0) {
			cexp++;
			cfrac >>= 1;
		}

		cfrac += (0x1 << (64 - FLOAT64_FRACTION_SIZE - 3));

		if (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1)))) {
			result.parts.fraction =
			    ((cfrac >> (64 - FLOAT64_FRACTION_SIZE - 2)) & (~FLOAT64_HIDDEN_BIT_MASK));
			return result;
		}
	} else {
		cfrac += (0x1 << (64 - FLOAT64_FRACTION_SIZE - 3));
	}

	++cexp;

	if (cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1))) {
		++cexp;
		cfrac >>= 1;
	}

	/* check overflow */
	if (cexp >= FLOAT64_MAX_EXPONENT) {
		/* FIXME: overflow, return infinity */
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		result.parts.fraction = 0;
		return result;
	}

	result.parts.exp = (uint32_t) cexp;

	result.parts.fraction =
	    ((cfrac >> (64 - FLOAT64_FRACTION_SIZE - 2)) & (~FLOAT64_HIDDEN_BIT_MASK));

	return result;
}

/**
 * Take fraction, round it, normalize it and detect exceptions
 *
 * @param cexp Exponent with bias.
 * @param cfrac_hi High part of the fraction shifted 14 bits to the left
 *     with added hidden bit.
 * @param cfrac_lo Low part of the fraction shifted 14 bits to the left
 *     with added hidden bit.
 * @param sign Resulting sign.
 * @param shift_out Bits right-shifted out from fraction by the caller.
 * @return Finished quadruple-precision float.
 */
float128 finish_float128(int32_t cexp, uint64_t cfrac_hi, uint64_t cfrac_lo,
    char sign, uint64_t shift_out)
{
	float128 result;
	uint64_t tmp_hi, tmp_lo;

	result.parts.sign = sign;

	/* find first nonzero digit and shift result and detect possibly underflow */
	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    1, &tmp_hi, &tmp_lo);
	and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	while ((cexp > 0) && (lt128(0x0ll, 0x0ll, cfrac_hi, cfrac_lo)) &&
	    (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo))) {
		cexp--;
		lshift128(cfrac_hi, cfrac_lo, 1, &cfrac_hi, &cfrac_lo);
		/* TODO: fix underflow */

		lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    1, &tmp_hi, &tmp_lo);
		and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	}

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    1, &tmp_hi, &tmp_lo);
	and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if ((cexp < 0) || (cexp == 0 &&
	    (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)))) {
		/* FIXME: underflow */
		result.parts.exp = 0;
		if ((cexp + FLOAT128_FRACTION_SIZE + 1) < 0) { /* +1 is place for rounding */
			result.parts.frac_hi = 0x0ll;
			result.parts.frac_lo = 0x0ll;
			return result;
		}

		while (cexp < 0) {
			cexp++;
			rshift128(cfrac_hi, cfrac_lo, 1, &cfrac_hi, &cfrac_lo);
		}

		if (shift_out & (0x1ull < 64)) {
			add128(cfrac_hi, cfrac_lo, 0x0ll, 0x1ll, &cfrac_hi, &cfrac_lo);
		}

		lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
		    1, &tmp_hi, &tmp_lo);
		and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
		if (!lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
			not128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
			    &tmp_hi, &tmp_lo);
			and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
			result.parts.frac_hi = tmp_hi;
			result.parts.frac_lo = tmp_lo;
			return result;
		}
	} else {
		if (shift_out & (0x1ull < 64)) {
			add128(cfrac_hi, cfrac_lo, 0x0ll, 0x1ll, &cfrac_hi, &cfrac_lo);
		}
	}

	++cexp;

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    1, &tmp_hi, &tmp_lo);
	and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
		++cexp;
		rshift128(cfrac_hi, cfrac_lo, 1, &cfrac_hi, &cfrac_lo);
	}

	/* check overflow */
	if (cexp >= FLOAT128_MAX_EXPONENT) {
		/* FIXME: overflow, return infinity */
		result.parts.exp = FLOAT128_MAX_EXPONENT;
		result.parts.frac_hi = 0x0ll;
		result.parts.frac_lo = 0x0ll;
		return result;
	}

	result.parts.exp = (uint32_t) cexp;

	not128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    &tmp_hi, &tmp_lo);
	and128(cfrac_hi, cfrac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	result.parts.frac_hi = tmp_hi;
	result.parts.frac_lo = tmp_lo;

	return result;
}

/**
 * Counts leading zeroes in byte.
 *
 * @param i Byte for which to count leading zeroes.
 * @return Number of detected leading zeroes.
 */
int count_zeroes8(uint8_t i)
{
	return zeroTable[i];
}

/**
 * Counts leading zeroes in 32bit unsigned integer.
 *
 * @param i Integer for which to count leading zeroes.
 * @return Number of detected leading zeroes.
 */
int count_zeroes32(uint32_t i)
{
	int j;
	for (j = 0; j < 32; j += 8) {
		if (i & (0xFF << (24 - j))) {
			return (j + count_zeroes8(i >> (24 - j)));
		}
	}

	return 32;
}

/**
 * Counts leading zeroes in 64bit unsigned integer.
 *
 * @param i Integer for which to count leading zeroes.
 * @return Number of detected leading zeroes.
 */
int count_zeroes64(uint64_t i)
{
	int j;
	for (j = 0; j < 64; j += 8) {
		if (i & (0xFFll << (56 - j))) {
			return (j + count_zeroes8(i >> (56 - j)));
		}
	}

	return 64;
}

/**
 * Round and normalize number expressed by exponent and fraction with
 * first bit (equal to hidden bit) at 30th bit.
 *
 * @param exp Exponent part.
 * @param fraction Fraction with hidden bit shifted to 30th bit.
 */
void round_float32(int32_t *exp, uint32_t *fraction)
{
	/* rounding - if first bit after fraction is set then round up */
	(*fraction) += (0x1 << (32 - FLOAT32_FRACTION_SIZE - 3));

	if ((*fraction) &
	    (FLOAT32_HIDDEN_BIT_MASK << (32 - FLOAT32_FRACTION_SIZE - 1))) {
		/* rounding overflow */
		++(*exp);
		(*fraction) >>= 1;
	}

	if (((*exp) >= FLOAT32_MAX_EXPONENT) || ((*exp) < 0)) {
		/* overflow - set infinity as result */
		(*exp) = FLOAT32_MAX_EXPONENT;
		(*fraction) = 0;
	}
}

/**
 * Round and normalize number expressed by exponent and fraction with
 * first bit (equal to hidden bit) at bit 62.
 *
 * @param exp Exponent part.
 * @param fraction Fraction with hidden bit shifted to bit 62.
 */
void round_float64(int32_t *exp, uint64_t *fraction)
{
	/*
	 * Rounding - if first bit after fraction is set then round up.
	 */

	/*
	 * Add 1 to the least significant bit of the fraction respecting the
	 * current shift to bit 62 and see if there will be a carry to bit 63.
	 */
	(*fraction) += (0x1 << (64 - FLOAT64_FRACTION_SIZE - 3));

	/* See if there was a carry to bit 63. */
	if ((*fraction) &
	    (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1))) {
		/* rounding overflow */
		++(*exp);
		(*fraction) >>= 1;
	}

	if (((*exp) >= FLOAT64_MAX_EXPONENT) || ((*exp) < 0)) {
		/* overflow - set infinity as result */
		(*exp) = FLOAT64_MAX_EXPONENT;
		(*fraction) = 0;
	}
}

/**
 * Round and normalize number expressed by exponent and fraction with
 * first bit (equal to hidden bit) at 126th bit.
 *
 * @param exp Exponent part.
 * @param frac_hi High part of fraction part with hidden bit shifted to 126th bit.
 * @param frac_lo Low part of fraction part with hidden bit shifted to 126th bit.
 */
void round_float128(int32_t *exp, uint64_t *frac_hi, uint64_t *frac_lo)
{
	uint64_t tmp_hi, tmp_lo;

	/* rounding - if first bit after fraction is set then round up */
	lshift128(0x0ll, 0x1ll, (128 - FLOAT128_FRACTION_SIZE - 3), &tmp_hi, &tmp_lo);
	add128(*frac_hi, *frac_lo, tmp_hi, tmp_lo, frac_hi, frac_lo);

	lshift128(FLOAT128_HIDDEN_BIT_MASK_HI, FLOAT128_HIDDEN_BIT_MASK_LO,
	    (128 - FLOAT128_FRACTION_SIZE - 3), &tmp_hi, &tmp_lo);
	and128(*frac_hi, *frac_lo, tmp_hi, tmp_lo, &tmp_hi, &tmp_lo);
	if (lt128(0x0ll, 0x0ll, tmp_hi, tmp_lo)) {
		/* rounding overflow */
		++(*exp);
		rshift128(*frac_hi, *frac_lo, 1, frac_hi, frac_lo);
	}

	if (((*exp) >= FLOAT128_MAX_EXPONENT) || ((*exp) < 0)) {
		/* overflow - set infinity as result */
		(*exp) = FLOAT128_MAX_EXPONENT;
		(*frac_hi) = 0;
		(*frac_lo) = 0;
	}
}

/**
 * Logical shift left on the 128-bit operand.
 *
 * @param a_hi High part of the input operand.
 * @param a_lo Low part of the input operand.
 * @param shift Number of bits by witch to shift.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void lshift128(
    uint64_t a_hi, uint64_t a_lo, int shift,
    uint64_t *r_hi, uint64_t *r_lo)
{
	if (shift <= 0) {
		/* do nothing */
	} else if (shift >= 128) {
		a_hi = 0;
		a_lo = 0;
	} else if (shift >= 64) {
		a_hi = a_lo << (shift - 64);
		a_lo = 0;
	} else {
		a_hi <<= shift;
		a_hi |= a_lo >> (64 - shift);
		a_lo <<= shift;
	}

	*r_hi = a_hi;
	*r_lo = a_lo;
}

/**
 * Logical shift right on the 128-bit operand.
 *
 * @param a_hi High part of the input operand.
 * @param a_lo Low part of the input operand.
 * @param shift Number of bits by witch to shift.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void rshift128(
    uint64_t a_hi, uint64_t a_lo, int shift,
    uint64_t *r_hi, uint64_t *r_lo)
{
	if (shift <= 0) {
		/* do nothing */
	} else 	if (shift >= 128) {
		a_hi = 0;
		a_lo = 0;
	} else if (shift >= 64) {
		a_lo = a_hi >> (shift - 64);
		a_hi = 0;
	} else {
		a_lo >>= shift;
		a_lo |= a_hi << (64 - shift);
		a_hi >>= shift;
	}

	*r_hi = a_hi;
	*r_lo = a_lo;
}

/**
 * Bitwise AND on 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void and128(
    uint64_t a_hi, uint64_t a_lo,
    uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hi, uint64_t *r_lo)
{
	*r_hi = a_hi & b_hi;
	*r_lo = a_lo & b_lo;
}

/**
 * Bitwise inclusive OR on 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void or128(
    uint64_t a_hi, uint64_t a_lo,
    uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hi, uint64_t *r_lo)
{
	*r_hi = a_hi | b_hi;
	*r_lo = a_lo | b_lo;
}

/**
 * Bitwise exclusive OR on 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void xor128(
    uint64_t a_hi, uint64_t a_lo,
    uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hi, uint64_t *r_lo)
{
	*r_hi = a_hi ^ b_hi;
	*r_lo = a_lo ^ b_lo;
}

/**
 * Bitwise NOT on the 128-bit operand.
 *
 * @param a_hi High part of the input operand.
 * @param a_lo Low part of the input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void not128(
    uint64_t a_hi, uint64_t a_lo,
	uint64_t *r_hi, uint64_t *r_lo)
{
	*r_hi = ~a_hi;
	*r_lo = ~a_lo;
}

/**
 * Equality comparison of 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @return 1 if operands are equal, 0 otherwise.
 */
int eq128(uint64_t a_hi, uint64_t a_lo, uint64_t b_hi, uint64_t b_lo)
{
	return (a_hi == b_hi) && (a_lo == b_lo);
}

/**
 * Lower-or-equal comparison of 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @return 1 if a is lower or equal to b, 0 otherwise.
 */
int le128(uint64_t a_hi, uint64_t a_lo, uint64_t b_hi, uint64_t b_lo)
{
	return (a_hi < b_hi) || ((a_hi == b_hi) && (a_lo <= b_lo));
}

/**
 * Lower-than comparison of 128-bit operands.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @return 1 if a is lower than b, 0 otherwise.
 */
int lt128(uint64_t a_hi, uint64_t a_lo, uint64_t b_hi, uint64_t b_lo)
{
	return (a_hi < b_hi) || ((a_hi == b_hi) && (a_lo < b_lo));
}

/**
 * Addition of two 128-bit unsigned integers.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void add128(uint64_t a_hi, uint64_t a_lo,
    uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hi, uint64_t *r_lo)
{
	uint64_t low = a_lo + b_lo;
	*r_lo = low;
	/* detect overflow to add a carry */
	*r_hi = a_hi + b_hi + (low < a_lo);
}

/**
 * Substraction of two 128-bit unsigned integers.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void sub128(uint64_t a_hi, uint64_t a_lo,
    uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hi, uint64_t *r_lo)
{
	*r_lo = a_lo - b_lo;
	/* detect underflow to substract a carry */
	*r_hi = a_hi - b_hi - (a_lo < b_lo);
}

/**
 * Multiplication of two 64-bit unsigned integers.
 *
 * @param a First input operand.
 * @param b Second input operand.
 * @param r_hi Address to store high part of the result.
 * @param r_lo Address to store low part of the result.
 */
void mul64(uint64_t a, uint64_t b, uint64_t *r_hi, uint64_t *r_lo)
{
	uint64_t low, high, middle1, middle2;
	uint32_t alow, blow;

	alow = a & 0xFFFFFFFF;
	blow = b & 0xFFFFFFFF;

	a >>= 32;
	b >>= 32;

	low = ((uint64_t) alow) * blow;
	middle1 = a * blow;
	middle2 = alow * b;
	high = a * b;

	middle1 += middle2;
	high += (((uint64_t) (middle1 < middle2)) << 32) + (middle1 >> 32);
	middle1 <<= 32;
	low += middle1;
	high += (low < middle1);
	*r_lo = low;
	*r_hi = high;
}

/**
 * Multiplication of two 128-bit unsigned integers.
 *
 * @param a_hi High part of the first input operand.
 * @param a_lo Low part of the first input operand.
 * @param b_hi High part of the second input operand.
 * @param b_lo Low part of the second input operand.
 * @param r_hihi Address to store first (highest) quarter of the result.
 * @param r_hilo Address to store second quarter of the result.
 * @param r_lohi Address to store third quarter of the result.
 * @param r_lolo Address to store fourth (lowest) quarter of the result.
 */
void mul128(uint64_t a_hi, uint64_t a_lo, uint64_t b_hi, uint64_t b_lo,
    uint64_t *r_hihi, uint64_t *r_hilo, uint64_t *r_lohi, uint64_t *r_lolo)
{
	uint64_t hihi, hilo, lohi, lolo;
	uint64_t tmp1, tmp2;

	mul64(a_lo, b_lo, &lohi, &lolo);
	mul64(a_lo, b_hi, &hilo, &tmp2);
	add128(hilo, tmp2, 0x0ll, lohi, &hilo, &lohi);
	mul64(a_hi, b_hi, &hihi, &tmp1);
	add128(hihi, tmp1, 0x0ll, hilo, &hihi, &hilo);
	mul64(a_hi, b_lo, &tmp1, &tmp2);
	add128(tmp1, tmp2, 0x0ll, lohi, &tmp1, &lohi);
	add128(hihi, hilo, 0x0ll, tmp1, &hihi, &hilo);

	*r_hihi = hihi;
	*r_hilo = hilo;
	*r_lohi = lohi;
	*r_lolo = lolo;
}

/**
 * Estimate the quotient of 128-bit unsigned divident and 64-bit unsigned
 * divisor.
 *
 * @param a_hi High part of the divident.
 * @param a_lo Low part of the divident.
 * @param b Divisor.
 * @return Quotient approximation.
 */
uint64_t div128est(uint64_t a_hi, uint64_t a_lo, uint64_t b)
{
	uint64_t b_hi, b_lo;
	uint64_t rem_hi, rem_lo;
	uint64_t tmp_hi, tmp_lo;
	uint64_t result;

	if (b <= a_hi) {
		return 0xFFFFFFFFFFFFFFFFull;
	}

	b_hi = b >> 32;
	result = ((b_hi << 32) <= a_hi) ? (0xFFFFFFFFull << 32) : (a_hi / b_hi) << 32;
	mul64(b, result, &tmp_hi, &tmp_lo);
	sub128(a_hi, a_lo, tmp_hi, tmp_lo, &rem_hi, &rem_lo);

	while ((int64_t) rem_hi < 0) {
		result -= 0x1ll << 32;
		b_lo = b << 32;
		add128(rem_hi, rem_lo, b_hi, b_lo, &rem_hi, &rem_lo);
	}

	rem_hi = (rem_hi << 32) | (rem_lo >> 32);
	if ((b_hi << 32) <= rem_hi) {
		result |= 0xFFFFFFFF;
	} else {
		result |= rem_hi / b_hi;
	}

	return result;
}

/** @}
 */
