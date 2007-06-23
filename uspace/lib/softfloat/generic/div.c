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
#include<add.h>
#include<div.h>
#include<comparison.h>
#include<mul.h>
#include<common.h>


float32 divFloat32(float32 a, float32 b) 
{
	float32 result;
	int32_t aexp, bexp, cexp;
	uint64_t afrac, bfrac, cfrac;
	
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
	
	/* find first nonzero digit and shift result and detect possibly underflow */
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
		result.parts.exp = (uint32_t)cexp;
	}
	
	result.parts.fraction = ((cfrac >> 6) & (~FLOAT32_HIDDEN_BIT_MASK)); 
	
	return result;	
}

float64 divFloat64(float64 a, float64 b) 
{
	float64 result;
	int64_t aexp, bexp, cexp;
	uint64_t afrac, bfrac, cfrac; 
	uint64_t remlo, remhi;
	
	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat64NaN(a)) {
		
		if (isFloat64SigNaN(b)) {
			/*FIXME: SigNaN*/
			return b;
		}
		
		if (isFloat64SigNaN(a)) {
			/*FIXME: SigNaN*/
		}
		/*NaN*/
		return a;
	}
	
	if (isFloat64NaN(b)) {
		if (isFloat64SigNaN(b)) {
			/*FIXME: SigNaN*/
		}
		/*NaN*/
		return b;
	}
	
	if (isFloat64Infinity(a)) {
		if (isFloat64Infinity(b) || isFloat64Zero(b)) {
			/*FIXME: inf / inf */
			result.binary = FLOAT64_NAN;
			return result;
		}
		/* inf / num */
		result.parts.exp = a.parts.exp;
		result.parts.fraction = a.parts.fraction;
		return result;
	}

	if (isFloat64Infinity(b)) {
		if (isFloat64Zero(a)) {
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
	
	if (isFloat64Zero(b)) {
		if (isFloat64Zero(a)) {
			/*FIXME: 0 / 0*/
			result.binary = FLOAT64_NAN;
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
		while (! (afrac & FLOAT64_HIDDEN_BIT_MASK) ) {
			afrac <<= 1;
			aexp--;
		}
	}

	if (bexp == 0) {
		bexp++;
			/* bfrac is nonzero => it must stop */	
		while (! (bfrac & FLOAT64_HIDDEN_BIT_MASK) ) {
			bfrac <<= 1;
			bexp--;
		}
	}

	afrac =	(afrac | FLOAT64_HIDDEN_BIT_MASK ) << (64 - FLOAT64_FRACTION_SIZE - 2 );
	bfrac =	(bfrac | FLOAT64_HIDDEN_BIT_MASK ) << (64 - FLOAT64_FRACTION_SIZE - 1);

	if ( bfrac <= (afrac << 1) ) {
		afrac >>= 1;
		aexp++;
	}
	
	cexp = aexp - bexp + FLOAT64_BIAS - 2; 
	
	cfrac = divFloat64estim(afrac, bfrac);
	
	if ((  cfrac & 0x1FF ) <= 2) { /*FIXME:?? */
		mul64integers( bfrac, cfrac, &remlo, &remhi);
		/* (__u128)afrac << 64 - ( ((__u128)remhi<<64) + (__u128)remlo )*/	
		remhi = afrac - remhi - ( remlo > 0);
		remlo = - remlo;
		
		while ((int64_t) remhi < 0) {
			cfrac--;
			remlo += bfrac;
			remhi += ( remlo < bfrac );
		}
		cfrac |= ( remlo != 0 );
	}
	
	/* round and shift */
	result = finishFloat64(cexp, cfrac, result.parts.sign);
	return result;

}

uint64_t divFloat64estim(uint64_t a, uint64_t b)
{
	uint64_t bhi;
	uint64_t remhi, remlo;
	uint64_t result;
	
	if ( b <= a ) {
		return 0xFFFFFFFFFFFFFFFFull;
	}
	
	bhi = b >> 32;
	result = ((bhi << 32) <= a) ?( 0xFFFFFFFFull << 32) : ( a / bhi) << 32;
	mul64integers(b, result, &remlo, &remhi);
	
	remhi = a - remhi - (remlo > 0);
	remlo = - remlo;

	b <<= 32;
	while ( (int64_t) remhi < 0 ) {
			result -= 0x1ll << 32;	
			remlo += b;
			remhi += bhi + ( remlo < b );
		}
	remhi = (remhi << 32) | (remlo >> 32);
	if (( bhi << 32) <= remhi) {
		result |= 0xFFFFFFFF;
	} else {
		result |= remhi / bhi;
	}
	
	
	return result;
}

/** @}
 */
