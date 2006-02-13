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
#include<common.h>

/** Take fraction shifted by 10 bits to left, round it, normalize it and detect exceptions
 * @param exp exponent with bias
 * @param cfrac fraction shifted 10 places left with added hidden bit
 * @return valied float64
 */
float64 finishFloat64(__s32 cexp, __u64 cfrac, char sign)
{
	float64 result;

	result.parts.sign = sign;

	/* find first nonzero digit and shift result and detect possibly underflow */
	while ((cexp > 0) && (cfrac) && (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1 ) )))) {
		cexp--; 
		cfrac <<= 1;
			/* TODO: fix underflow */
	};
	
	cfrac += (0x1 << (64 - FLOAT64_FRACTION_SIZE - 3)); 
	
	if ((cexp < 0) || ( cexp == 0 && (!(cfrac & (FLOAT64_HIDDEN_BIT_MASK << (64 - FLOAT64_FRACTION_SIZE - 1)))))) {
		/* FIXME: underflow */
		result.parts.exp = 0;
		if ((cexp + FLOAT64_FRACTION_SIZE) < 0) {
			result.parts.fraction = 0;
			return result;
		}
		//cfrac >>= 1;
		while (cexp < 0) {
			cexp++;
			cfrac >>= 1;
		}
			
		result.parts.fraction = ((cfrac >>(64 - FLOAT64_FRACTION_SIZE - 2) ) & (~FLOAT64_HIDDEN_BIT_MASK)); 

		return result;
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

	result.parts.exp = (__u32)cexp;
	
	result.parts.fraction = ((cfrac >>(64 - FLOAT64_FRACTION_SIZE - 2 ) ) & (~FLOAT64_HIDDEN_BIT_MASK)); 
	
	return result;	
}

