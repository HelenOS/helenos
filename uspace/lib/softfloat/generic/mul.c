/*
 * Copyright (c) 2005 Josef Cejka
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
/** @file
 */

#include<sftypes.h>
#include<mul.h>
#include<comparison.h>
#include<common.h>

/** Multiply two 32 bit float numbers
 *
 */
float32 mulFloat32(float32 a, float32 b)
{
	float32 result;
	uint64_t frac1, frac2;
	int32_t exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat32NaN(a) || isFloat32NaN(b) ) {
		/* TODO: fix SigNaNs */
		if (isFloat32SigNaN(a)) {
			result.parts.fraction = a.parts.fraction;
			result.parts.exp = a.parts.exp;
			return result;
		};
		if (isFloat32SigNaN(b)) { /* TODO: fix SigNaN */
			result.parts.fraction = b.parts.fraction;
			result.parts.exp = b.parts.exp;
			return result;
		};
		/* set NaN as result */
		result.binary = FLOAT32_NAN;
		return result;
	};
		
	if (isFloat32Infinity(a)) { 
		if (isFloat32Zero(b)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT32_NAN;
			return result;
		}
		result.parts.fraction = a.parts.fraction;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (isFloat32Infinity(b)) { 
		if (isFloat32Zero(a)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT32_NAN;
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
		result.binary = FLOAT32_INF;
		result.parts.sign = a.parts.sign ^ b.parts.sign;
		return result;
	};
	
	if (exp < 0) { 
		/* FIXME: underflow */
		/* return signed zero */
		result.parts.fraction = 0x0;
		result.parts.exp = 0x0;
		return result;
	};
	
	frac1 = a.parts.fraction;
	if (a.parts.exp > 0) {
		frac1 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};
	
	frac2 = b.parts.fraction;

	if (b.parts.exp > 0) {
		frac2 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};

	frac1 <<= 1; /* one bit space for rounding */

	frac1 = frac1 * frac2;
/* round and return */
	
	while ((exp < FLOAT32_MAX_EXPONENT) && (frac1 >= ( 1 << (FLOAT32_FRACTION_SIZE + 2)))) { 
		/* 23 bits of fraction + one more for hidden bit (all shifted 1 bit left)*/
		++exp;
		frac1 >>= 1;
	};

	/* rounding */
	/* ++frac1; FIXME: not works - without it is ok */
	frac1 >>= 1; /* shift off rounding space */
	
	if ((exp < FLOAT32_MAX_EXPONENT) && (frac1 >= (1 << (FLOAT32_FRACTION_SIZE + 1)))) {
		++exp;
		frac1 >>= 1;
	};

	if (exp >= FLOAT32_MAX_EXPONENT ) {	
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
		};
		if (frac1 == 0) {
			/* FIXME : underflow */
		result.parts.exp = 0;
		result.parts.fraction = 0;
		return result;
		};
	};
	result.parts.exp = exp; 
	result.parts.fraction = frac1 & ( (1 << FLOAT32_FRACTION_SIZE) - 1);
	
	return result;	
	
}

/** Multiply two 64 bit float numbers
 *
 */
float64 mulFloat64(float64 a, float64 b)
{
	float64 result;
	uint64_t frac1, frac2;
	int32_t exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat64NaN(a) || isFloat64NaN(b) ) {
		/* TODO: fix SigNaNs */
		if (isFloat64SigNaN(a)) {
			result.parts.fraction = a.parts.fraction;
			result.parts.exp = a.parts.exp;
			return result;
		};
		if (isFloat64SigNaN(b)) { /* TODO: fix SigNaN */
			result.parts.fraction = b.parts.fraction;
			result.parts.exp = b.parts.exp;
			return result;
		};
		/* set NaN as result */
		result.binary = FLOAT64_NAN;
		return result;
	};
		
	if (isFloat64Infinity(a)) { 
		if (isFloat64Zero(b)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT64_NAN;
			return result;
		}
		result.parts.fraction = a.parts.fraction;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (isFloat64Infinity(b)) { 
		if (isFloat64Zero(a)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT64_NAN;
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
	};
	
	frac2 = b.parts.fraction;

	if (b.parts.exp > 0) {
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};

	frac1 <<= (64 - FLOAT64_FRACTION_SIZE - 1);
	frac2 <<= (64 - FLOAT64_FRACTION_SIZE - 2);

	mul64integers(frac1, frac2, &frac1, &frac2);

	frac2 |= (frac1 != 0);
	if (frac2 & (0x1ll << 62)) {
		frac2 <<= 1;
		exp--;
	}

	result = finishFloat64(exp, frac2, result.parts.sign);
	return result;
}

/** Multiply two 64 bit numbers and return result in two parts
 * @param a first operand
 * @param b second operand
 * @param lo lower part from result
 * @param hi higher part of result
 */
void mul64integers(uint64_t a,uint64_t b, uint64_t *lo, uint64_t *hi)
{
	uint64_t low, high, middle1, middle2;
	uint32_t alow, blow;

	alow = a & 0xFFFFFFFF;
	blow = b & 0xFFFFFFFF;
	
	a >>= 32;
	b >>= 32;
	
	low = ((uint64_t)alow) * blow;
	middle1 = a * blow;
	middle2 = alow * b;
	high = a * b;

	middle1 += middle2;
	high += (((uint64_t)(middle1 < middle2)) << 32) + (middle1 >> 32);
	middle1 <<= 32;
	low += middle1;
	high += (low < middle1);
	*lo = low;
	*hi = high;
	
	return;
}

/** @}
 */
