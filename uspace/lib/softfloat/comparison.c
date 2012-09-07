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
/** @file Comparison functions.
 */

#include "sftypes.h"
#include "comparison.h"
#include "common.h"

/**
 * Determines whether the given float represents NaN (either signalling NaN or
 * quiet NaN).
 *
 * @param f Single-precision float.
 * @return 1 if float is NaN, 0 otherwise.
 */
int is_float32_nan(float32 f)
{
	/* NaN : exp = 0xff and nonzero fraction */
	return ((f.parts.exp == 0xFF) && (f.parts.fraction));
}

/**
 * Determines whether the given float represents NaN (either signalling NaN or
 * quiet NaN).
 *
 * @param d Double-precision float.
 * @return 1 if float is NaN, 0 otherwise.
 */
int is_float64_nan(float64 d)
{
	/* NaN : exp = 0x7ff and nonzero fraction */
	return ((d.parts.exp == 0x7FF) && (d.parts.fraction));
}

/**
 * Determines whether the given float represents NaN (either signalling NaN or
 * quiet NaN).
 *
 * @param ld Quadruple-precision float.
 * @return 1 if float is NaN, 0 otherwise.
 */
int is_float128_nan(float128 ld)
{
	/* NaN : exp = 0x7fff and nonzero fraction */
	return ((ld.parts.exp == 0x7FF) &&
	    !eq128(ld.parts.frac_hi, ld.parts.frac_lo, 0x0ll, 0x0ll));
}

/**
 * Determines whether the given float represents signalling NaN.
 *
 * @param f Single-precision float.
 * @return 1 if float is signalling NaN, 0 otherwise.
 */
int is_float32_signan(float32 f)
{
	/* SigNaN : exp = 0xff and fraction = 0xxxxx..x (binary),
	 * where at least one x is nonzero */
	return ((f.parts.exp == 0xFF) &&
	    (f.parts.fraction < 0x400000) && (f.parts.fraction));
}

/**
 * Determines whether the given float represents signalling NaN.
 *
 * @param d Double-precision float.
 * @return 1 if float is signalling NaN, 0 otherwise.
 */
int is_float64_signan(float64 d)
{
	/* SigNaN : exp = 0x7ff and fraction = 0xxxxx..x (binary),
	 * where at least one x is nonzero */
	return ((d.parts.exp == 0x7FF) &&
	    (d.parts.fraction) && (d.parts.fraction < 0x8000000000000ll));
}

/**
 * Determines whether the given float represents signalling NaN.
 *
 * @param ld Quadruple-precision float.
 * @return 1 if float is signalling NaN, 0 otherwise.
 */
int is_float128_signan(float128 ld)
{
	/* SigNaN : exp = 0x7fff and fraction = 0xxxxx..x (binary),
	 * where at least one x is nonzero */
	return ((ld.parts.exp == 0x7FFF) &&
	    (ld.parts.frac_hi || ld.parts.frac_lo) &&
	    lt128(ld.parts.frac_hi, ld.parts.frac_lo, 0x800000000000ll, 0x0ll));

}

/**
 * Determines whether the given float represents positive or negative infinity.
 *
 * @param f Single-precision float.
 * @return 1 if float is infinite, 0 otherwise.
 */
int is_float32_infinity(float32 f)
{
	/* NaN : exp = 0x7ff and zero fraction */
	return ((f.parts.exp == 0xFF) && (f.parts.fraction == 0x0));
}

/**
 * Determines whether the given float represents positive or negative infinity.
 *
 * @param d Double-precision float.
 * @return 1 if float is infinite, 0 otherwise.
 */
int is_float64_infinity(float64 d)
{
	/* NaN : exp = 0x7ff and zero fraction */
	return ((d.parts.exp == 0x7FF) && (d.parts.fraction == 0x0));
}

/**
 * Determines whether the given float represents positive or negative infinity.
 *
 * @param ld Quadruple-precision float.
 * @return 1 if float is infinite, 0 otherwise.
 */
int is_float128_infinity(float128 ld)
{
	/* NaN : exp = 0x7fff and zero fraction */
	return ((ld.parts.exp == 0x7FFF) &&
	    eq128(ld.parts.frac_hi, ld.parts.frac_lo, 0x0ll, 0x0ll));
}

/**
 * Determines whether the given float represents positive or negative zero.
 *
 * @param f Single-precision float.
 * @return 1 if float is zero, 0 otherwise.
 */
int is_float32_zero(float32 f)
{
	return (((f.bin) & 0x7FFFFFFF) == 0);
}

/**
 * Determines whether the given float represents positive or negative zero.
 *
 * @param d Double-precision float.
 * @return 1 if float is zero, 0 otherwise.
 */
int is_float64_zero(float64 d)
{
	return (((d.bin) & 0x7FFFFFFFFFFFFFFFll) == 0);
}

/**
 * Determines whether the given float represents positive or negative zero.
 *
 * @param ld Quadruple-precision float.
 * @return 1 if float is zero, 0 otherwise.
 */
int is_float128_zero(float128 ld)
{
	uint64_t tmp_hi;
	uint64_t tmp_lo;
	
	and128(ld.bin.hi, ld.bin.lo,
	    0x7FFFFFFFFFFFFFFFll, 0xFFFFFFFFFFFFFFFFll, &tmp_hi, &tmp_lo);
	
	return eq128(tmp_hi, tmp_lo, 0x0ll, 0x0ll);
}

/**
 * Determine whether two floats are equal. NaNs are not recognized.
 *
 * @a First single-precision operand.
 * @b Second single-precision operand.
 * @return 1 if both floats are equal, 0 otherwise.
 */
int is_float32_eq(float32 a, float32 b)
{
	/* a equals to b or both are zeros (with any sign) */
	return ((a.bin == b.bin) ||
	    (((a.bin | b.bin) & 0x7FFFFFFF) == 0));
}

/**
 * Determine whether two floats are equal. NaNs are not recognized.
 *
 * @a First double-precision operand.
 * @b Second double-precision operand.
 * @return 1 if both floats are equal, 0 otherwise.
 */
int is_float64_eq(float64 a, float64 b)
{
	/* a equals to b or both are zeros (with any sign) */
	return ((a.bin == b.bin) ||
	    (((a.bin | b.bin) & 0x7FFFFFFFFFFFFFFFll) == 0));
}

/**
 * Determine whether two floats are equal. NaNs are not recognized.
 *
 * @a First quadruple-precision operand.
 * @b Second quadruple-precision operand.
 * @return 1 if both floats are equal, 0 otherwise.
 */
int is_float128_eq(float128 a, float128 b)
{
	uint64_t tmp_hi;
	uint64_t tmp_lo;
	
	/* both are zeros (with any sign) */
	or128(a.bin.hi, a.bin.lo,
	    b.bin.hi, b.bin.lo, &tmp_hi, &tmp_lo);
	and128(tmp_hi, tmp_lo,
	    0x7FFFFFFFFFFFFFFFll, 0xFFFFFFFFFFFFFFFFll, &tmp_hi, &tmp_lo);
	int both_zero = eq128(tmp_hi, tmp_lo, 0x0ll, 0x0ll);
	
	/* a equals to b */
	int are_equal = eq128(a.bin.hi, a.bin.lo, b.bin.hi, b.bin.lo);
	
	return are_equal || both_zero;
}

/**
 * Lower-than comparison between two floats. NaNs are not recognized.
 *
 * @a First single-precision operand.
 * @b Second single-precision operand.
 * @return 1 if a is lower than b, 0 otherwise.
 */
int is_float32_lt(float32 a, float32 b) 
{
	if (((a.bin | b.bin) & 0x7FFFFFFF) == 0) {
		/* +- zeroes */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, smaller is that with greater binary value */
		return (a.bin > b.bin);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return (a.bin < b.bin);
}

/**
 * Lower-than comparison between two floats. NaNs are not recognized.
 *
 * @a First double-precision operand.
 * @b Second double-precision operand.
 * @return 1 if a is lower than b, 0 otherwise.
 */
int is_float64_lt(float64 a, float64 b)
{
	if (((a.bin | b.bin) & 0x7FFFFFFFFFFFFFFFll) == 0) {
		/* +- zeroes */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, smaller is that with greater binary value */
		return (a.bin > b.bin);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return (a.bin < b.bin);
}

/**
 * Lower-than comparison between two floats. NaNs are not recognized.
 *
 * @a First quadruple-precision operand.
 * @b Second quadruple-precision operand.
 * @return 1 if a is lower than b, 0 otherwise.
 */
int is_float128_lt(float128 a, float128 b)
{
	uint64_t tmp_hi;
	uint64_t tmp_lo;
	
	or128(a.bin.hi, a.bin.lo,
	    b.bin.hi, b.bin.lo, &tmp_hi, &tmp_lo);
	and128(tmp_hi, tmp_lo,
	    0x7FFFFFFFFFFFFFFFll, 0xFFFFFFFFFFFFFFFFll, &tmp_hi, &tmp_lo);
	if (eq128(tmp_hi, tmp_lo, 0x0ll, 0x0ll)) {
		/* +- zeroes */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, smaller is that with greater binary value */
		return lt128(b.bin.hi, b.bin.lo, a.bin.hi, a.bin.lo);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return lt128(a.bin.hi, a.bin.lo, b.bin.hi, b.bin.lo);
}

/**
 * Greater-than comparison between two floats. NaNs are not recognized.
 *
 * @a First single-precision operand.
 * @b Second single-precision operand.
 * @return 1 if a is greater than b, 0 otherwise.
 */
int is_float32_gt(float32 a, float32 b) 
{
	if (((a.bin | b.bin) & 0x7FFFFFFF) == 0) {
		/* zeroes are equal with any sign */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, greater is that with smaller binary value */
		return (a.bin < b.bin);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return (a.bin > b.bin);
}

/**
 * Greater-than comparison between two floats. NaNs are not recognized.
 *
 * @a First double-precision operand.
 * @b Second double-precision operand.
 * @return 1 if a is greater than b, 0 otherwise.
 */
int is_float64_gt(float64 a, float64 b)
{
	if (((a.bin | b.bin) & 0x7FFFFFFFFFFFFFFFll) == 0) {
		/* zeroes are equal with any sign */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, greater is that with smaller binary value */
		return (a.bin < b.bin);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return (a.bin > b.bin);
}

/**
 * Greater-than comparison between two floats. NaNs are not recognized.
 *
 * @a First quadruple-precision operand.
 * @b Second quadruple-precision operand.
 * @return 1 if a is greater than b, 0 otherwise.
 */
int is_float128_gt(float128 a, float128 b)
{
	uint64_t tmp_hi;
	uint64_t tmp_lo;
	
	or128(a.bin.hi, a.bin.lo,
	    b.bin.hi, b.bin.lo, &tmp_hi, &tmp_lo);
	and128(tmp_hi, tmp_lo,
	    0x7FFFFFFFFFFFFFFFll, 0xFFFFFFFFFFFFFFFFll, &tmp_hi, &tmp_lo);
	if (eq128(tmp_hi, tmp_lo, 0x0ll, 0x0ll)) {
		/* zeroes are equal with any sign */
		return 0;
	}
	
	if ((a.parts.sign) && (b.parts.sign)) {
		/* if both are negative, greater is that with smaller binary value */
		return lt128(a.bin.hi, a.bin.lo, b.bin.hi, b.bin.lo);
	}
	
	/*
	 * lets negate signs - now will be positive numbers always
	 * bigger than negative (first bit will be set for unsigned
	 * integer comparison)
	 */
	a.parts.sign = !a.parts.sign;
	b.parts.sign = !b.parts.sign;
	return lt128(b.bin.hi, b.bin.lo, a.bin.hi, a.bin.lo);
}

/** @}
 */
