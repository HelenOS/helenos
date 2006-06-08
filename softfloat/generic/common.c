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
#include<common.h>

/* Table for fast leading zeroes counting */
char zeroTable[256] = {
	8, 7, 7, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4, \
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, \
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, \
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/** Take fraction shifted by 10 bits to left, round it, normalize it and detect exceptions
 * @param cexp exponent with bias
 * @param cfrac fraction shifted 10 places left with added hidden bit
 * @param sign
 * @return valied float64
 */
float64 finishFloat64(int32_t cexp, uint64_t cfrac, char sign)
{
	float64 result;

	result.parts.sign = sign;

	/* find first nonzero digit and shift result and detect possibly underflow */
	while ((cexp > 0) && (cfrac) && (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1 ) )))) {
		cexp--; 
		cfrac <<= 1;
			/* TODO: fix underflow */
	};
	
	if ((cexp < 0) || ( cexp == 0 && (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1)))))) {
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
			
			result.parts.fraction = ((cfrac >>(64 - FLOAT64_FRACTION_SIZE - 2) ) & (~FLOAT64_HIDDEN_BIT_MASK)); 
			return result;
		}	
	} else {
		cfrac += (0x1 << (64 - FLOAT64_FRACTION_SIZE - 3)); 
	}
	
	++cexp;

	if (cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1 ))) {
		++cexp;
		cfrac >>= 1;
	}	

	/* check overflow */
	if (cexp >= FLOAT64_MAX_EXPONENT ) {
		/* FIXME: overflow, return infinity */
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		result.parts.fraction = 0;
		return result;
	}

	result.parts.exp = (uint32_t)cexp;
	
	result.parts.fraction = ((cfrac >>(64 - FLOAT64_FRACTION_SIZE - 2 ) ) & (~FLOAT64_HIDDEN_BIT_MASK)); 
	
	return result;	
}

/** Counts leading zeroes in 64bit unsigned integer
 * @param i 
 */
int countZeroes64(uint64_t i)
{
	int j;
	for (j =0; j < 64; j += 8) {
		if ( i & (0xFFll << (56 - j))) {
			return (j + countZeroes8(i >> (56 - j)));
		}
	}

	return 64;
}

/** Counts leading zeroes in 32bit unsigned integer
 * @param i 
 */
int countZeroes32(uint32_t i)
{
	int j;
	for (j =0; j < 32; j += 8) {
		if ( i & (0xFF << (24 - j))) {
			return (j + countZeroes8(i >> (24 - j)));
		}
	}

	return 32;
}

/** Counts leading zeroes in byte
 * @param i 
 */
int countZeroes8(uint8_t i)
{
	return zeroTable[i];
}

/** Round and normalize number expressed by exponent and fraction with first bit (equal to hidden bit) at 30. bit
 * @param exp exponent 
 * @param fraction part with hidden bit shifted to 30. bit
 */
void roundFloat32(int32_t *exp, uint32_t *fraction)
{
	/* rounding - if first bit after fraction is set then round up */
	(*fraction) += (0x1 << 6);
	
	if ((*fraction) & (FLOAT32_HIDDEN_BIT_MASK << 8)) { 
		/* rounding overflow */
		++(*exp);
		(*fraction) >>= 1;
	};
	
	if (((*exp) >= FLOAT32_MAX_EXPONENT ) || ((*exp) < 0)) {
		/* overflow - set infinity as result */
		(*exp) = FLOAT32_MAX_EXPONENT;
		(*fraction) = 0;
		return;
	}

	return;
}

/** Round and normalize number expressed by exponent and fraction with first bit (equal to hidden bit) at 62. bit
 * @param exp exponent 
 * @param fraction part with hidden bit shifted to 62. bit
 */
void roundFloat64(int32_t *exp, uint64_t *fraction)
{
	/* rounding - if first bit after fraction is set then round up */
	(*fraction) += (0x1 << 9);
	
	if ((*fraction) & (FLOAT64_HIDDEN_BIT_MASK << 11)) { 
		/* rounding overflow */
		++(*exp);
		(*fraction) >>= 1;
	};
	
	if (((*exp) >= FLOAT64_MAX_EXPONENT ) || ((*exp) < 0)) {
		/* overflow - set infinity as result */
		(*exp) = FLOAT64_MAX_EXPONENT;
		(*fraction) = 0;
		return;
	}

	return;
}


 /** @}
 */

