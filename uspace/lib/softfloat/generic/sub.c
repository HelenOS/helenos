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
#include<sub.h>
#include<comparison.h>

/** Subtract two float32 numbers with same signs
 */
float32 subFloat32(float32 a, float32 b)
{
	int expdiff;
	uint32_t exp1, exp2, frac1, frac2;
	float32 result;

	result.f = 0;
	
	expdiff = a.parts.exp - b.parts.exp;
	if ((expdiff < 0 ) || ((expdiff == 0) && (a.parts.fraction < b.parts.fraction))) {
		if (isFloat32NaN(b)) {
			/* TODO: fix SigNaN */
			if (isFloat32SigNaN(b)) {
			};
			return b;
		};
		
		if (b.parts.exp == FLOAT32_MAX_EXPONENT) { 
			b.parts.sign = !b.parts.sign; /* num -(+-inf) = -+inf */
			return b;
		}
		
		result.parts.sign = !a.parts.sign; 
		
		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (isFloat32NaN(a)) {
			/* TODO: fix SigNaN */
			if (isFloat32SigNaN(a) || isFloat32SigNaN(b)) {
			};
			return a;
		};
		
		if (a.parts.exp == FLOAT32_MAX_EXPONENT) { 
			if (b.parts.exp == FLOAT32_MAX_EXPONENT) {
				/* inf - inf => nan */
				/* TODO: fix exception */
				result.binary = FLOAT32_NAN;
				return result;
			};
			return a;
		}
		
		result.parts.sign = a.parts.sign;
		
		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;	
	};
	
	if (exp1 == 0) {
		/* both are denormalized */
		result.parts.fraction = frac1-frac2;
		if (result.parts.fraction > frac1) {
			/* TODO: underflow exception */
			return result;
		};
		result.parts.exp = 0;
		return result;
	};

	/* add hidden bit */
	frac1 |= FLOAT32_HIDDEN_BIT_MASK; 
	
	if (exp2 == 0) {
		/* denormalized */
		--expdiff;	
	} else {
		/* normalized */
		frac2 |= FLOAT32_HIDDEN_BIT_MASK;
	};
	
	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;
	
	if (expdiff > FLOAT32_FRACTION_SIZE + 1) {
	     goto done;	
	     };
	
	frac1 = frac1 - (frac2 >> expdiff);
done:
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	while ((exp1 > 0) && (!(frac1 & (FLOAT32_HIDDEN_BIT_MASK << 6 )))) {
		--exp1;
		frac1 <<= 1;
			/* TODO: fix underflow - frac1 == 0 does not necessary means underflow... */
	};
	
	/* rounding - if first bit after fraction is set then round up */
	frac1 += 0x20;

	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	};
	
	/*Clear hidden bit and shift */
	result.parts.fraction = ((frac1 >> 6) & (~FLOAT32_HIDDEN_BIT_MASK)); 
	result.parts.exp = exp1;
	
	return result;
}

/** Subtract two float64 numbers with same signs
 */
float64 subFloat64(float64 a, float64 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1, frac2;
	float64 result;

	result.d = 0;
	
	expdiff = a.parts.exp - b.parts.exp;
	if ((expdiff < 0 ) || ((expdiff == 0) && (a.parts.fraction < b.parts.fraction))) {
		if (isFloat64NaN(b)) {
			/* TODO: fix SigNaN */
			if (isFloat64SigNaN(b)) {
			};
			return b;
		};
		
		if (b.parts.exp == FLOAT64_MAX_EXPONENT) { 
			b.parts.sign = !b.parts.sign; /* num -(+-inf) = -+inf */
			return b;
		}
		
		result.parts.sign = !a.parts.sign; 
		
		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if (isFloat64NaN(a)) {
			/* TODO: fix SigNaN */
			if (isFloat64SigNaN(a) || isFloat64SigNaN(b)) {
			};
			return a;
		};
		
		if (a.parts.exp == FLOAT64_MAX_EXPONENT) { 
			if (b.parts.exp == FLOAT64_MAX_EXPONENT) {
				/* inf - inf => nan */
				/* TODO: fix exception */
				result.binary = FLOAT64_NAN;
				return result;
			};
			return a;
		}
		
		result.parts.sign = a.parts.sign;
		
		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;	
	};
	
	if (exp1 == 0) {
		/* both are denormalized */
		result.parts.fraction = frac1 - frac2;
		if (result.parts.fraction > frac1) {
			/* TODO: underflow exception */
			return result;
		};
		result.parts.exp = 0;
		return result;
	};

	/* add hidden bit */
	frac1 |= FLOAT64_HIDDEN_BIT_MASK; 
	
	if (exp2 == 0) {
		/* denormalized */
		--expdiff;	
	} else {
		/* normalized */
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	};
	
	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;
	
	if (expdiff > FLOAT64_FRACTION_SIZE + 1) {
	     goto done;	
	     };
	
	frac1 = frac1 - (frac2 >> expdiff);
done:
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	while ((exp1 > 0) && (!(frac1 & (FLOAT64_HIDDEN_BIT_MASK << 6 )))) {
		--exp1;
		frac1 <<= 1;
			/* TODO: fix underflow - frac1 == 0 does not necessary means underflow... */
	};
	
	/* rounding - if first bit after fraction is set then round up */
	frac1 += 0x20;

	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7)) {
		++exp1;
		frac1 >>= 1;
	};
	
	/*Clear hidden bit and shift */
	result.parts.fraction = ((frac1 >> 6) & (~FLOAT64_HIDDEN_BIT_MASK)); 
	result.parts.exp = exp1;
	
	return result;
}


 /** @}
 */

