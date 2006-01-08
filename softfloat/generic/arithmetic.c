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
#include<arithmetic.h>
#include<comparison.h>

/** Add two Float32 numbers with same signs
 */
float32 addFloat32(float32 a, float32 b)
{
	int expdiff;
	__u32 exp1,exp2,mant1,mant2;
	
	expdiff=a.parts.exp - b.parts.exp;
	if (expdiff<0) {
		if (isFloat32NaN(b)) {
			//TODO: fix SigNaN
			if (isFloat32SigNaN(b)) {
			};

			return b;
		};
		
		if (b.parts.exp==0xFF) { 
			return b;
		}
		
		mant1=b.parts.mantisa;
		exp1=b.parts.exp;
		mant2=a.parts.mantisa;
		exp2=a.parts.exp;
		expdiff*=-1;
	} else {
		if (isFloat32NaN(a)) {
			//TODO: fix SigNaN
			if ((isFloat32SigNaN(a))||(isFloat32SigNaN(b))) {
			};
			return a;
		};
		
		if (a.parts.exp==0xFF) { 
			return a;
		}
		
		mant1=a.parts.mantisa;
		exp1=a.parts.exp;
		mant2=b.parts.mantisa;
		exp2=b.parts.exp;
	};
	
	if (exp1==0) {
		//both are denormalized
		mant1+=mant2;
		if (mant1&0xF00000) {
			a.parts.exp=1;
		};
		a.parts.mantisa=mant1;
		return a;
	};
	
	// create some space for rounding
	mant1<<=6;
	mant2<<=6;
	
	mant1|=0x20000000; //add hidden bit
	
	
	if (exp2==0) {
		--expdiff;	
	} else {
		mant2|=0x20000000; //hidden bit
	};
	
	if (expdiff>24) {
	     goto done;	
	     };
	
	mant2>>=expdiff;
	mant1+=mant2;
done:
	if (mant1&0x40000000) {
		++exp1;
		mant1>>=1;
	};
	
	//rounding - if first bit after mantisa is set then round up
	mant1+=0x20;
	
	if (mant1&0x40000000) { 
		++exp1;
		mant1>>=1;
	};
	
	a.parts.exp=exp1;
	a.parts.mantisa = ((mant1&(~0x20000000))>>6); /*Clear hidden bit and shift */
	return a;
};

/** Subtract two float32 numbers with same signs
 */
float32 subFloat32(float32 a, float32 b)
{
	int expdiff;
	__u32 exp1,exp2,mant1,mant2;
	float32 result;

	result.f = 0;
	
	expdiff=a.parts.exp - b.parts.exp;
	if ((expdiff<0)||((expdiff==0)&&(a.parts.mantisa<b.parts.mantisa))) {
		if (isFloat32NaN(b)) {
			//TODO: fix SigNaN
			if (isFloat32SigNaN(b)) {
			};
			return b;
		};
		
		if (b.parts.exp==0xFF) { 
			b.parts.sign=!b.parts.sign; /* num -(+-inf) = -+inf */
			return b;
		}
		
		result.parts.sign = !a.parts.sign; 
		
		mant1=b.parts.mantisa;
		exp1=b.parts.exp;
		mant2=a.parts.mantisa;
		exp2=a.parts.exp;
		expdiff*=-1;
	} else {
		if (isFloat32NaN(a)) {
			//TODO: fix SigNaN
			if ((isFloat32SigNaN(a))||(isFloat32SigNaN(b))) {
			};
			return a;
		};
		
		if (a.parts.exp==0xFF) { 
			if (b.parts.exp==0xFF) {
				/* inf - inf => nan */
				//TODO: fix exception
				result.binary = FLOAT32_NAN;
				return result;
			};
			return a;
		}
		
		result.parts.sign = a.parts.sign;
		
		mant1=a.parts.mantisa;
		exp1=a.parts.exp;
		mant2=b.parts.mantisa;
		exp2=b.parts.exp;
		

		
	};
	
	if (exp1==0) {
		//both are denormalized
		result.parts.mantisa=mant1-mant2;
		if (result.parts.mantisa>mant1) {
			//TODO: underflow exception
			return result;
		};
		result.parts.exp=0;
		return result;
	};
	
	// create some space for rounding
	mant1<<=6;
	mant2<<=6;
	
	mant1|=0x20000000; //add hidden bit
	
	
	if (exp2==0) {
		--expdiff;	
	} else {
		mant2|=0x20000000; //hidden bit
	};
	
	if (expdiff>24) {
	     goto done;	
	     };
	
	mant1 = mant1-(mant2>>expdiff);
done:
	
	//TODO: find first nonzero digit and shift result and detect possibly underflow
	while ((exp1>0)&&(!(mant1&0x20000000))) {
		exp1--;
		mant1 <<= 1;
		if(mant1 == 0) {
			/* Realy is it an underflow? ... */
			/* TODO: fix underflow */
		};
	};
	
	//rounding - if first bit after mantisa is set then round up	
	mant1 += 0x20;

	if (mant1&0x40000000) {
		++exp1;
		mant1>>=1;
	};
	
	result.parts.mantisa = ((mant1&(~0x20000000))>>6); /*Clear hidden bit and shift */
	result.parts.exp = exp1;
	
	return result;
};

/** Multiply two 32 bit float numbers
 *
 */
float32 mulFloat32(float32 a, float32 b)
{
	float32 result;
	__u64 mant1, mant2;
	__s32 exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if ((isFloat32NaN(a))||(isFloat32NaN(b))) {
		/* TODO: fix SigNaNs */
		if (isFloat32SigNaN(a)) {
			result.parts.mantisa = a.parts.mantisa;
			result.parts.exp = a.parts.exp;
			return result;
		};
		if (isFloat32SigNaN(b)) { /* TODO: fix SigNaN */
			result.parts.mantisa = b.parts.mantisa;
			result.parts.exp = b.parts.exp;
			return result;
		};
		/* set NaN as result */
		result.parts.mantisa = 0x1;
		result.parts.exp = 0xFF;
		return result;
	};
		
	if (isFloat32Infinity(a)) { 
		if (isFloat32Zero(b)) {
			/* FIXME: zero * infinity */
			result.parts.mantisa = 0x1;
			result.parts.exp = 0xFF;
			return result;
		}
		result.parts.mantisa = a.parts.mantisa;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (isFloat32Infinity(b)) { 
		if (isFloat32Zero(a)) {
			/* FIXME: zero * infinity */
			result.parts.mantisa = 0x1;
			result.parts.exp = 0xFF;
			return result;
		}
		result.parts.mantisa = b.parts.mantisa;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp;
	exp -= FLOAT32_BIAS;
	
	if (exp >= 0xFF ) {
		/* FIXME: overflow */
		/* set infinity as result */
		result.parts.mantisa = 0x0;
		result.parts.exp = 0xFF;
		return result;
	};
	
	if (exp < 0) { 
		/* FIXME: underflow */
		/* return signed zero */
		result.parts.mantisa = 0x0;
		result.parts.exp = 0x0;
		return result;
	};
	
	mant1 = a.parts.mantisa;
	if (a.parts.exp>0) {
		mant1 |= 0x800000;
	} else {
		++exp;
	};
	
	mant2 = b.parts.mantisa;
	if (b.parts.exp>0) {
		mant2 |= 0x800000;
	} else {
		++exp;
	};

	mant1 <<= 1; /* one bit space for rounding */

	mant1 = mant1 * mant2;
/* round and return */
	
	while ((exp < 0xFF )&&(mant1 > 0x1FFFFFF )) { /* 0xFFFFFF is 23 bits of mantisa + one more for hidden bit (all shifted 1 bit left)*/
		++exp;
		mant1 >>= 1;
	};

	/* rounding */
	//++mant1; /* FIXME: not works - without it is ok */
	mant1 >>= 1; /* shift off rounding space */
	
	if ((exp < 0xFF )&&(mant1 > 0xFFFFFF )) {
		++exp;
		mant1 >>= 1;
	};

	if (exp >= 0xFF ) {	
		/* TODO: fix overflow */
		/* return infinity*/
		result.parts.exp = 0xFF;
		result.parts.mantisa = 0x0;
		return result;
	}
	
	exp -= FLOAT32_MANTISA_SIZE;

	if (exp <= FLOAT32_MANTISA_SIZE) { 
		/* denormalized number */
		mant1 >>= 1; /* denormalize */
		while ((mant1 > 0) && (exp < 0)) {
			mant1 >>= 1;
			++exp;
		};
		if (mant1 == 0) {
			/* FIXME : underflow */
		result.parts.exp = 0;
		result.parts.mantisa = 0;
		return result;
		};
	};
	result.parts.exp = exp; 
	result.parts.mantisa = mant1 & 0x7FFFFF;
	
	return result;	
	
};


