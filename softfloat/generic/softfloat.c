
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

#include<softfloat.h>

float32 addFloat32(float32 a, float32 b);
float32 subFloat32(float32 a, float32 b);
inline int isFloat32NaN(float32 f);
inline int isFloat32SigNaN(float32 f);

float __addsf3(float a, float b)
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	if (fa.parts.sign!=fb.parts.sign) return subFloat32(fa,fb).f;
	return addFloat32(fa,fb).f;
};

float __subsf3(float a, float b)
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	if (fa.parts.sign!=fb.parts.sign) return addFloat32(fa,fb).f;
	return subFloat32(fa,fb).f;
};

float __negsf2(float a)
{
	float32 fa;
	fa.f=a;
	fa.parts.sign=!fa.parts.sign;
	return fa.f;
};

double __negdf2(double a)
{
	float64 fa;
	fa.d=a;
	fa.parts.sign=!fa.parts.sign;
	return fa.d;
};

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
	
	a.parts.exp=exp1;
	a.parts.mantisa=mant1>>6;
	return a;
};

/** Substract two float32 numbers with same signs
 */
float32 subFloat32(float32 a, float32 b)
{
	
	
};

inline int isFloat32NaN(float32 f)
{	/* NaN : exp = 0xff and nonzero mantisa */
	return ((f.parts.exp==0xFF)&&(f.parts.mantisa));
};

inline int isFloat32SigNaN(float32 f)
{	/* SigNaN : exp = 0xff mantisa = 1xxxxx..x (binary), where at least one x is nonzero */
	return ((f.parts.exp==0xFF)&&(f.parts.mantisa>0x400000));
};

