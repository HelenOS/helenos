
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


float __addsf3(float a, float b)
{
	float32 fa, fb;
	a.f=a;
	b.f=b;
	if (a.parts.sign!=b.parts.sign) return subFloat32(a,b).f;
	return addFloat32(a,b).f;
};

float __subsf3(float a, float b)
{
	float32 fa, fb;
	a.f=a;
	b.f=b;
	if (a.parts.sign!=b.parts.sign) return addFloat32(a,b).f;
	return subFloat32(a,b).f;
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
	fa.f=a;
	fa.parts.sign=!fa.parts.sign;
	return fa.f;
};

float32 addFloat32(float32 a, float32 b)
{
	__u32 expdiff;
	__u32 exp1,exp2,mant1,mant2;
	
	expdiff=a.parts.exp - b.parts.exp;
	if (expdiff<0) {
		if (isFloat32NaN(a)) {
			//TODO: fix it
			return a;
		};

		mant1=b.parts.mantisa;
		exp1=b.parts.exp;
		mant2=a.parts.mantisa;
		exp2=a.parts.exp;
		expdiff*=-1;
	} else {
		if (isFloat32NaN(b)) {
			//TODO: fix it
			return b;
		};
		mant1=a.parts.mantisa;
		exp1=a.parts.exp;
		mant2=b.parts.mantisa;
		exp2=b.parts.exp;
	};

	// create some space for rounding
	mant1<<=6;
	mant2<<=6;
	
	if (exp1!=0) {
		mant1|=0x20000000; //add hidden bit
	};
	
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
	//TODO: round mant1
	a.parts.exp=exp1;
	a.parts.mantisa=mant1>>6;
	return a;
};

float32 subFloat32(float32 a, float32 b)
{
	
	
};

inline int isFloat32NaN(float32 f)
{	/* NaN : exp = 0xff and nonzero mantisa */
	float32 fa;
	fa.f=f;
	return ((fa.parts.exp==0xFF)&&(fa.parts.mantisa));
};

inline int isFloat32SigNaN(float32 f)
{	/* SigNaN : exp = 0xff mantisa = 1xxxxx..x (binary), where at least one x is nonzero */
	float32 fa;
	fa.f=f;
	return ((fa.parts.exp==0xFF)&&(fa.parts.mantisa>0x400000));
};

