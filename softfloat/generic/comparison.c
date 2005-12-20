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
{	/* NaN : exp = 0xff and nonzero mantisa */
	return ((f.parts.exp==0xFF)&&(f.parts.mantisa));
};

inline int isFloat32SigNaN(float32 f)
{	/* SigNaN : exp = 0xff mantisa = 1xxxxx..x (binary), where at least one x is nonzero */
	return ((f.parts.exp==0xFF)&&(f.parts.mantisa>0x400000));
};

inline int isFloat32Infinity(float32 f) 
{
	return ((f.parts.exp==0xFF)&&(f.parts.mantisa==0x0));
};

/**
 * @return 1, if both floats are equal - but NaNs are not recognized 
 */
inline int isFloat32eq(float32 a, float32 b)
{
	return ((a==b)||(((a.binary| b.binary)&0x7FFFFFFF)==0)); /* a equals to b or both are zeros (with any sign) */
}

/**
 * @return 1, if a<b - but NaNs are not recognized 
 */
inline int isFloat32lt(float32 a, float32 b) 
{
	if (((a.binary| b.binary)&0x7FFFFFFF)==0) {
		return 0;
	};
	a.parts.sign^=a.parts.sign;
	b.parts.sign^=b.parts.sign;
	return (a.binary<b.binary);
			
}

/**
 * @return 1, if a>b - but NaNs are not recognized 
 */
inline int isFloat32gt(float32 a, float32 b) 
{
	if (((a.binary| b.binary)&0x7FFFFFFF)==0) {
		return 0;
	};
	a.parts.sign^=a.parts.sign;
	b.parts.sign^=b.parts.sign;
	return (a.binary>b.binary);
			
}


