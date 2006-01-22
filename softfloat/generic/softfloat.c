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
#include<sftypes.h>

#include<add.h>
#include<sub.h>
#include<mul.h>
#include<div.h>

#include<conversion.h>
#include<comparison.h>
#include<other.h>

/* Arithmetic functions */

float __addsf3(float a, float b)
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	if (fa.parts.sign!=fb.parts.sign) {
		if (fa.parts.sign) {
			fa.parts.sign=0;
			return subFloat32(fb,fa).f;
		};
		fb.parts.sign=0;
		return subFloat32(fa,fb).f;
	}
	return addFloat32(fa,fb).f;
}

float __subsf3(float a, float b)
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	if (fa.parts.sign!=fb.parts.sign) {
		fb.parts.sign=!fb.parts.sign;
		return addFloat32(fa,fb).f;
	}
	return subFloat32(fa,fb).f;
}

float __mulsf3(float a, float b) 
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	return 	mulFloat32(fa, fb).f;
}

float __divsf3(float a, float b) 
{
	float32 fa, fb;
	fa.f=a;
	fb.f=b;
	//return 	divFloat32(fa, fb).f;
}

float __negsf2(float a)
{
	float32 fa;
	fa.f=a;
	fa.parts.sign=!fa.parts.sign;
	return fa.f;
}

double __negdf2(double a)
{
	float64 fa;
	fa.d=a;
	fa.parts.sign=!fa.parts.sign;
	return fa.d;
}

/* Conversion functions */

double __extendsfdf2(float a) 
{
	float32 fa;
	fa.f = a;
	return convertFloat32ToFloat64(fa).d;
}

float __truncdfsf2(double a) 
{
	float64 da;
	da.d = a;
	return convertFloat64ToFloat32(da).f;
}

/* Comparison functions */

/* a<b .. -1
 * a=b ..  0
 * a>b ..  1
 * */

int __cmpsf2(float a, float b) 
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		return 1; /* no special constant for unordered - maybe signaled? */
	};

	
	if (isFloat32eq(fa,fb)) {
		return 0;
	};
	
	if (isFloat32lt(fa,fb)) {
		return -1;
		};
	return 1;
}

int __unordsf2(float a, float b) 
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	return ((isFloat32NaN(fa))||(isFloat32NaN(fb)));
}

/** 
 * @return zero, if neither argument is a NaN and are equal
 * */
int __eqsf2(float a, float b) 
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		/* TODO: sigNaNs*/
		return 1;
		};
	return isFloat32eq(fa,fb)-1;
}

/* strange behavior, but it was in gcc documentation */
int __nesf2(float a, float b) 
{
	return __eqsf2(a,b);
}

/* return value >= 0 if a>=b and neither is NaN */
int __gesf2(float a, float b)
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		/* TODO: sigNaNs*/
		return -1;
		};
	
	if (isFloat32eq(fa,fb)) {
		return 0;
	};
	
	if (isFloat32gt(fa,fb)) {
		return 1;
		};
	
	return -1;
}

/** Return negative value, if a<b and neither is NaN*/
int __ltsf2(float a, float b)
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		/* TODO: sigNaNs*/
		return 1;
		};
	if (isFloat32lt(fa, fb)) {
		return -1;
		};
	return 0;
}

/* return value <= 0 if a<=b and neither is NaN */
int __lesf2(float a, float b)
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		/* TODO: sigNaNs*/
		return 1;
		};
	
	if (isFloat32eq(fa,fb)) {
		return 0;
	};
	
	if (isFloat32lt(fa,fb)) {
		return -1;
		};
	
	return 1;
}

/** Return positive value, if a>b and neither is NaN*/
int __gtsf2(float a, float b)
{
	float32 fa,fb;
	fa.f=a;
	fb.f=b;
	if ((isFloat32NaN(fa))||(isFloat32NaN(fb))) {
		/* TODO: sigNaNs*/
		return -1;
		};
	if (isFloat32gt(fa, fb)) {
		return 1;
		};
	return 0;
}

/* Other functions */

float __powisf2(float a, int b)
{
//TODO:	
}

float __mulsc3(float a, float b, float c, float d)
{
//TODO:
}

float __divsc3(float a, float b, float c, float d)
{
//TODO:
}

