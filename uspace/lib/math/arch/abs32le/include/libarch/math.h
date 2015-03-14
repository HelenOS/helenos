/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup libmathabs32le
 * @{
 */
/** @file
 */

#ifndef LIBMATH_abs32le_MATH_H_
#define LIBMATH_abs32le_MATH_H_

#include <mathtypes.h>
#include <mod.h>
#include <trunc.h>
#include <trig.h>

static inline float64_t fmod(float64_t dividend, float64_t divisor)
{
	return float64_mod(dividend, divisor);
}

static inline float64_t trunc(float64_t val)
{
	float64_u arg;
	arg.val = val;
	
	float64_u ret;
	ret.data = trunc_float64(arg.data);
	
	return ret.val;
}

static inline float64_t sin(float64_t val)
{
	return float64_sin(val);
}

static inline float64_t cos(float64_t val)
{
	return float64_cos(val);
}

#endif

/** @}
 */
