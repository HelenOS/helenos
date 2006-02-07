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
#include<comparison.h>

inline int isFloat32NaN(float32 f)
{	/* NaN : exp = 0xff and nonzero fraction */
	return ((f.parts.exp==0xFF)&&(f.parts.fraction));
};

inline int isFloat64NaN(float64 d)
{	/* NaN : exp = 0x7ff and nonzero fraction */
	return ((d.parts.exp==0x7FF)&&(d.parts.fraction));
};

inline int isFloat32SigNaN(float32 f)
{	/* SigNaN : exp = 0xff fraction = 0xxxxx..x (binary), where at least one x is nonzero */
	return ((f.parts.exp==0xFF)&&(f.parts.fraction<0x400000)&&(f.parts.fraction));
};

inline int isFloat64SigNaN(float64 d)
{	/* SigNaN : exp = 0x7ff fraction = 0xxxxx..x (binary), where at least one x is nonzero */
	return ((d.parts.exp==0x7FF)&&(d.parts.fraction)&&(d.parts.fraction<0x8000000000000ll));
};

inline int isFloat32Infinity(float32 f) 
{
	return ((f.parts.exp==0xFF)&&(f.parts.fraction==0x0));
};

inline int isFloat64Infinity(float64 d) 
{
	return ((d.parts.exp==0x7FF)&&(d.parts.fraction==0x0));
};

inline int isFloat32Zero(float32 f)
{
	return (((f.binary) & 0x7FFFFFFF) == 0);
}

inline int isFloat64Zero(float64 d)
{
	return (((d.binary) & 0x7FFFFFFFFFFFFFFFll) == 0);
}

/**
 * @return 1, if both floats are equal - but NaNs are not recognized 
 */
inline int isFloat32eq(float32 a, float32 b)
{
	return ((a.binary==b.binary)||(((a.binary| b.binary)&0x7FFFFFFF)==0)); /* a equals to b or both are zeros (with any sign) */
}

/**
 * @return 1, if a<b - but NaNs are not recognized 
 */
inline int isFloat32lt(float32 a, float32 b) 
{
	if (((a.binary| b.binary)&0x7FFFFFFF)==0) {
		return 0; /* +- zeroes */
	};
	
	if ((a.parts.sign)&&(b.parts.sign)) {
		/*if both are negative, smaller is that with greater binary value*/
		return (a.binary>b.binary);
		};
	
	/* lets negate signs - now will be positive numbers allways bigger than negative (first bit will be set for unsigned integer comparison)*/
	a.parts.sign=!a.parts.sign;
	b.parts.sign=!b.parts.sign;
	return (a.binary<b.binary);
			
}

/**
 * @return 1, if a>b - but NaNs are not recognized 
 */
inline int isFloat32gt(float32 a, float32 b) 
{
	if (((a.binary| b.binary)&0x7FFFFFFF)==0) {
		return 0; /* zeroes are equal with any sign */
	};
	
	if ((a.parts.sign)&&(b.parts.sign)) {
		/*if both are negative, greater is that with smaller binary value*/
		return (a.binary<b.binary);
		};
	
	/* lets negate signs - now will be positive numbers allways bigger than negative (first bit will be set for unsigned integer comparison)*/
	a.parts.sign=!a.parts.sign;
	b.parts.sign=!b.parts.sign;
	return (a.binary>b.binary);
			
}



