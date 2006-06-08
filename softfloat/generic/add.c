/*
 * Copyright (C) 2005 Josef Cejka
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
#include<add.h>
#include<comparison.h>

/** Add two Float32 numbers with same signs
 */
float32 addFloat32(float32 a, float32 b)
{
	int expdiff;
	uint32_t exp1, exp2,frac1, frac2;
	
	expdiff = a.parts.exp - b.parts.exp;
	if (expdiff < 0) {
		if (isFloat32NaN(b)) {
			/* TODO: fix SigNaN */
			if (isFloat32SigNaN(b)) {
			};

			return b;
		};
		
		if (b.parts.exp == FLOAT32_MAX_EXPONENT) { 
			return b;
		}
		
		frac1 = b.parts.fraction;
		exp1 = b.parts.exp;
		frac2 = a.parts.fraction;
		exp2 = a.parts.exp;
		expdiff *= -1;
	} else {
		if ((isFloat32NaN(a)) || (isFloat32NaN(b))) {
			/* TODO: fix SigNaN */
			if (isFloat32SigNaN(a) || isFloat32SigNaN(b)) {
			};
			return (isFloat32NaN(a)?a:b);
		};
		
		if (a.parts.exp == FLOAT32_MAX_EXPONENT) { 
			return a;
		}
		
		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	};
	
	if (exp1 == 0) {
		/* both are denormalized */
		frac1 += frac2;
		if (frac1 & FLOAT32_HIDDEN_BIT_MASK ) {
			/* result is not denormalized */
			a.parts.exp = 1;
		};
		a.parts.fraction = frac1;
		return a;
	};
	
	frac1 |= FLOAT32_HIDDEN_BIT_MASK; /* add hidden bit */

	if (exp2 == 0) {
		/* second operand is denormalized */
		--expdiff;
	} else {
		/* add hidden bit to second operand */
		frac2 |= FLOAT32_HIDDEN_BIT_MASK; 
	};
	
	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;
	
	if (expdiff < (FLOAT32_FRACTION_SIZE + 2) ) {
		frac2 >>= expdiff;
		frac1 += frac2;
	} else {
		a.parts.exp = exp1;
		a.parts.fraction = (frac1 >> 6) & (~(FLOAT32_HIDDEN_BIT_MASK));
		return a;
	}
	
	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7) ) {
		++exp1;
		frac1 >>= 1;
	};
	
	/* rounding - if first bit after fraction is set then round up */
	frac1 += (0x1 << 5);
	
	if (frac1 & (FLOAT32_HIDDEN_BIT_MASK << 7)) { 
		/* rounding overflow */
		++exp1;
		frac1 >>= 1;
	};
	
	
	if ((exp1 == FLOAT32_MAX_EXPONENT ) || (exp2 > exp1)) {
			/* overflow - set infinity as result */
			a.parts.exp = FLOAT32_MAX_EXPONENT;
			a.parts.fraction = 0;
			return a;
			}
	
	a.parts.exp = exp1;
	
	/*Clear hidden bit and shift */
	a.parts.fraction = ((frac1 >> 6) & (~FLOAT32_HIDDEN_BIT_MASK)) ; 
	return a;
}


/** Add two Float64 numbers with same signs
 */
float64 addFloat64(float64 a, float64 b)
{
	int expdiff;
	uint32_t exp1, exp2;
	uint64_t frac1, frac2;
	
	expdiff = ((int )a.parts.exp) - b.parts.exp;
	if (expdiff < 0) {
		if (isFloat64NaN(b)) {
			/* TODO: fix SigNaN */
			if (isFloat64SigNaN(b)) {
			};

			return b;
		};
		
		/* b is infinity and a not */	
		if (b.parts.exp == FLOAT64_MAX_EXPONENT ) { 
			return b;
		}
		
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
		
		/* a is infinity and b not */
		if (a.parts.exp == FLOAT64_MAX_EXPONENT ) { 
			return a;
		}
		
		frac1 = a.parts.fraction;
		exp1 = a.parts.exp;
		frac2 = b.parts.fraction;
		exp2 = b.parts.exp;
	};
	
	if (exp1 == 0) {
		/* both are denormalized */
		frac1 += frac2;
		if (frac1 & FLOAT64_HIDDEN_BIT_MASK) { 
			/* result is not denormalized */
			a.parts.exp = 1;
		};
		a.parts.fraction = frac1;
		return a;
	};
	
	/* add hidden bit - frac1 is sure not denormalized */
	frac1 |= FLOAT64_HIDDEN_BIT_MASK;

	/* second operand ... */
	if (exp2 == 0) {
		/* ... is denormalized */
		--expdiff;	
	} else {
		/* is not denormalized */
		frac2 |= FLOAT64_HIDDEN_BIT_MASK;
	};
	
	/* create some space for rounding */
	frac1 <<= 6;
	frac2 <<= 6;
	
	if (expdiff < (FLOAT64_FRACTION_SIZE + 2) ) {
		frac2 >>= expdiff;
		frac1 += frac2;
	} else {
		a.parts.exp = exp1;
		a.parts.fraction = (frac1 >> 6) & (~(FLOAT64_HIDDEN_BIT_MASK));
		return a;
	}
	
	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7) ) {
		++exp1;
		frac1 >>= 1;
	};
	
	/* rounding - if first bit after fraction is set then round up */
	frac1 += (0x1 << 5); 
	
	if (frac1 & (FLOAT64_HIDDEN_BIT_MASK << 7)) { 
		/* rounding overflow */
		++exp1;
		frac1 >>= 1;
	};
	
	if ((exp1 == FLOAT64_MAX_EXPONENT ) || (exp2 > exp1)) {
			/* overflow - set infinity as result */
			a.parts.exp = FLOAT64_MAX_EXPONENT;
			a.parts.fraction = 0;
			return a;
			}
	
	a.parts.exp = exp1;
	/*Clear hidden bit and shift */
	a.parts.fraction = ( (frac1 >> 6 ) & (~FLOAT64_HIDDEN_BIT_MASK));
	
	return a;
}



 /** @}
 */

