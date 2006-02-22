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

#include "sftypes.h"
#include "conversion.h"
#include "comparison.h"

float64 convertFloat32ToFloat64(float32 a) 
{
	float64 result;
	__u64 frac;
	
	result.parts.sign = a.parts.sign;
	result.parts.fraction = a.parts.fraction;
	result.parts.fraction <<= (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE );
	
	if ((isFloat32Infinity(a))||(isFloat32NaN(a))) {
		result.parts.exp = 0x7FF;
		/* TODO; check if its correct for SigNaNs*/
		return result;
	};
	
	result.parts.exp = a.parts.exp + ( (int)FLOAT64_BIAS - FLOAT32_BIAS );
	if (a.parts.exp == 0) {
		/* normalize denormalized numbers */

		if (result.parts.fraction == 0ll) { /* fix zero */
			result.parts.exp = 0ll;
			return result;
		}
			
		frac = result.parts.fraction;
		
		while (!(frac & (0x10000000000000ll))) {
			frac <<= 1;
			--result.parts.exp;
		};
		
		++result.parts.exp;
		result.parts.fraction = frac;
	};
	
	return result;
	
}

float32 convertFloat64ToFloat32(float64 a) 
{
	float32 result;
	__s32 exp;
	__u64 frac;
	
	result.parts.sign = a.parts.sign;
	
	if (isFloat64NaN(a)) {
		
		result.parts.exp = 0xFF;
		
		if (isFloat64SigNaN(a)) {
			result.parts.fraction = 0x800000; /* set first bit of fraction nonzero */
			return result;
		}
	
		result.parts.fraction = 0x1; /* fraction nonzero but its first bit is zero */
		return result;
	};

	if (isFloat64Infinity(a)) {
		result.parts.fraction = 0;
		result.parts.exp = 0xFF;
		return result;
	};

	exp = (int)a.parts.exp - FLOAT64_BIAS + FLOAT32_BIAS;
	
	if (exp >= 0xFF) {
		/*FIXME: overflow*/
		result.parts.fraction = 0;
		result.parts.exp = 0xFF;
		return result;
		
	} else if (exp <= 0 ) {
		
		/* underflow or denormalized */
		
		result.parts.exp = 0;
		
		exp *= -1;	
		if (exp > FLOAT32_FRACTION_SIZE ) {
			/* FIXME: underflow */
			result.parts.fraction = 0;
			return result;
		};
		
		/* denormalized */
		
		frac = a.parts.fraction; 
		frac |= 0x10000000000000ll; /* denormalize and set hidden bit */
		
		frac >>= (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE + 1);
		
		while (exp > 0) {
			--exp;
			frac >>= 1;
		};
		result.parts.fraction = frac;
		
		return result;
	};

	result.parts.exp = exp;
	result.parts.fraction = a.parts.fraction >> (FLOAT64_FRACTION_SIZE - FLOAT32_FRACTION_SIZE);
	return result;
}


/** Helping procedure for converting float32 to uint32
 * @param a floating point number in normalized form (no NaNs or Inf are checked )
 * @return unsigned integer
 */
static __u32 _float32_to_uint32_helper(float32 a)
{
	__u32 frac;
	
	if (a.parts.exp < FLOAT32_BIAS) {
		/*TODO: rounding*/
		return 0;
	}
	
	frac = a.parts.fraction;
	
	frac |= FLOAT32_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 32 - FLOAT32_FRACTION_SIZE - 1; 

	frac >>= 32 - (a.parts.exp - FLOAT32_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}
	
	return frac;
}

/* Convert float to unsigned int32
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__u32 float32_to_uint32(float32 a)
{
	if (isFloat32NaN(a)) {
		return MAX_UINT32;
	}
	
	if (isFloat32Infinity(a) || (a.parts.exp >= (32 + FLOAT32_BIAS)))  {
		if (a.parts.sign) {
			return MIN_UINT32;
		}
		return MAX_UINT32;
	}
	
	return _float32_to_uint32_helper(a);	
}

/* Convert float to signed int32
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__s32 float32_to_int32(float32 a)
{
	if (isFloat32NaN(a)) {
		return MAX_INT32;
	}
	
	if (isFloat32Infinity(a) || (a.parts.exp >= (32 + FLOAT32_BIAS)))  {
		if (a.parts.sign) {
			return MIN_INT32;
		}
		return MAX_INT32;
	}
	return _float32_to_uint32_helper(a);
}	


/** Helping procedure for converting float64 to uint64
 * @param a floating point number in normalized form (no NaNs or Inf are checked )
 * @return unsigned integer
 */
static __u64 _float64_to_uint64_helper(float64 a)
{
	__u64 frac;
	
	if (a.parts.exp < FLOAT64_BIAS) {
		/*TODO: rounding*/
		return 0;
	}
	
	frac = a.parts.fraction;
	
	frac |= FLOAT64_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 64 - FLOAT64_FRACTION_SIZE - 1; 

	frac >>= 64 - (a.parts.exp - FLOAT64_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}
	
	return frac;
}

/* Convert float to unsigned int64
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__u64 float64_to_uint64(float64 a)
{
	if (isFloat64NaN(a)) {
		return MAX_UINT64;
	}
	
	if (isFloat64Infinity(a) || (a.parts.exp >= (64 + FLOAT64_BIAS)))  {
		if (a.parts.sign) {
			return MIN_UINT64;
		}
		return MAX_UINT64;
	}
	
	return _float64_to_uint64_helper(a);	
}

/* Convert float to signed int64
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__s64 float64_to_int64(float64 a)
{
	if (isFloat64NaN(a)) {
		return MAX_INT64;
	}
	
	if (isFloat64Infinity(a) || (a.parts.exp >= (64 + FLOAT64_BIAS)))  {
		if (a.parts.sign) {
			return MIN_INT64;
		}
		return MAX_INT64;
	}
	return _float64_to_uint64_helper(a);
}	





/** Helping procedure for converting float32 to uint64
 * @param a floating point number in normalized form (no NaNs or Inf are checked )
 * @return unsigned integer
 */
static __u64 _float32_to_uint64_helper(float32 a)
{
	__u64 frac;
	
	if (a.parts.exp < FLOAT32_BIAS) {
		/*TODO: rounding*/
		return 0;
	}
	
	frac = a.parts.fraction;
	
	frac |= FLOAT32_HIDDEN_BIT_MASK;
	/* shift fraction to left so hidden bit will be the most significant bit */
	frac <<= 64 - FLOAT32_FRACTION_SIZE - 1; 

	frac >>= 64 - (a.parts.exp - FLOAT32_BIAS) - 1;
	if ((a.parts.sign == 1) && (frac != 0)) {
		frac = ~frac;
		++frac;
	}
	
	return frac;
}

/* Convert float to unsigned int64
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__u64 float32_to_uint64(float32 a)
{
	if (isFloat32NaN(a)) {
		return MAX_UINT64;
	}
	
	if (isFloat32Infinity(a) || (a.parts.exp >= (64 + FLOAT32_BIAS)))  {
		if (a.parts.sign) {
			return MIN_UINT64;
		}
		return MAX_UINT64;
	}
	
	return _float32_to_uint64_helper(a);	
}

/* Convert float to signed int64
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__s64 float32_to_int64(float32 a)
{
	if (isFloat32NaN(a)) {
		return MAX_INT64;
	}
	
	if (isFloat32Infinity(a) || (a.parts.exp >= (64 + FLOAT32_BIAS)))  {
		if (a.parts.sign) {
			return (MIN_INT64);
		}
		return MAX_INT64;
	}
	return _float32_to_uint64_helper(a);
}	


/* Convert float64 to unsigned int32
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__u32 float64_to_uint32(float64 a)
{
	if (isFloat64NaN(a)) {
		return MAX_UINT32;
	}
	
	if (isFloat64Infinity(a) || (a.parts.exp >= (32 + FLOAT64_BIAS)))  {
		if (a.parts.sign) {
			return MIN_UINT32;
		}
		return MAX_UINT32;
	}
	
	return (__u32)_float64_to_uint64_helper(a);	
}

/* Convert float64 to signed int32
 * FIXME: Im not sure what to return if overflow/underflow happens 
 * 	- now its the biggest or the smallest int
 */ 
__s32 float64_to_int32(float64 a)
{
	if (isFloat64NaN(a)) {
		return MAX_INT32;
	}
	
	if (isFloat64Infinity(a) || (a.parts.exp >= (32 + FLOAT64_BIAS)))  {
		if (a.parts.sign) {
			return MIN_INT32;
		}
		return MAX_INT32;
	}
	return (__s32)_float64_to_uint64_helper(a);
}	


