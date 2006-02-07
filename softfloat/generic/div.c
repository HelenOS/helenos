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

#include<sftypes.h>
#include<add.h>
#include<comparison.h>

float32 divFloat32(float32 a, float32 b) 
{
	float32 result;
	__s32 aexp, bexp, cexp;
	__u64 afrac, bfrac, cfrac;
	
	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat32NaN(a)) {
		if (isFloat32SigNaN(a)) {
			/*FIXME: SigNaN*/
		}
		/*NaN*/
		return a;
	}
	
	if (isFloat32NaN(b)) {
		if (isFloat32SigNaN(b)) {
			/*FIXME: SigNaN*/
		}
		/*NaN*/
		return b;
	}
	
	if (isFloat32Infinity(a)) {
		if (isFloat32Infinity(b)) {
			/*FIXME: inf / inf */
			result.binary = FLOAT32_NAN;
			return result;
		}
		/* inf / num */
		result.parts.exp = a.parts.exp;
		result.parts.fraction = a.parts.fraction;
		return result;
	}

	if (isFloat32Infinity(b)) {
		if (isFloat32Zero(a)) {
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
	
	if (isFloat32Zero(b)) {
		if (isFloat32Zero(a)) {
			/*FIXME: 0 / 0*/
			result.binary = FLOAT32_NAN;
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
		while (! (afrac & FLOAT32_HIDDEN_BIT_MASK) ) {
			afrac <<= 1;
			aexp--;
		}
	}

	if (bexp == 0) {
		bfrac <<= 1;
			/* bfrac is nonzero => it must stop */	
		while (! (bfrac & FLOAT32_HIDDEN_BIT_MASK) ) {
			bfrac <<= 1;
			bexp--;
		}
	}

	afrac =	(afrac | FLOAT32_HIDDEN_BIT_MASK ) << (32 - FLOAT32_FRACTION_SIZE - 1 );
	bfrac =	(bfrac | FLOAT32_HIDDEN_BIT_MASK ) << (32 - FLOAT32_FRACTION_SIZE );

	if ( bfrac <= (afrac << 1) ) {
		afrac >>= 1;
		aexp++;
	}
	
	cexp = aexp - bexp + FLOAT32_BIAS - 2;
	
	cfrac = (afrac << 32) / bfrac;
	if ((  cfrac & 0x3F ) == 0) { 
		cfrac |= ( bfrac * cfrac != afrac << 32 );
	}
	
	/* pack and round */
	
	/* TODO: find first nonzero digit and shift result and detect possibly underflow */
	while ((cexp > 0) && (cfrac) && (!(cfrac & (FLOAT32_HIDDEN_BIT_MASK << 7 )))) {
		cexp--;
		cfrac <<= 1;
			/* TODO: fix underflow */
	};
	
	
	cfrac += (0x1 << 6); /* FIXME: 7 is not sure*/
	
	if (cfrac & (FLOAT32_HIDDEN_BIT_MASK << 7)) {
		++cexp;
		cfrac >>= 1;
		}	

	/* check overflow */
	if (cexp >= FLOAT32_MAX_EXPONENT ) {
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
			cexp ++;
			cfrac >>= 1;
		}
		
	} else {
		result.parts.exp = (__u32)cexp;
	}
	
	result.parts.fraction = ((cfrac >> 6) & (~FLOAT32_HIDDEN_BIT_MASK)); 
	
	return result;	
}

