/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libmath
 * @{
 */
/** @file Mathematical operations.
 */

#ifndef LIBMATH_MATH_H_
#define LIBMATH_MATH_H_

#include <fmod.h>
#include <trig.h>
#include <trunc.h>

#define HUGE_VAL FLOAT64_INF

static inline float64_t cos_f64(float64_t val)
{
	return float64_cos(val);
}

static inline float32_t cos_f32(float32_t val)
{
	return float32_cos(val);
}

static inline float64_t fmod_f64(float64_t dividend, float64_t divisor)
{
	return float64_fmod(dividend, divisor);
}

static inline float64_t fmod_f32(float32_t dividend, float32_t divisor)
{
	return float32_fmod(dividend, divisor);
}

static inline float64_t sin_f64(float64_t val)
{
	return float64_sin(val);
}

static inline float32_t sin_f32(float32_t val)
{
	return float32_sin(val);
}

static inline float64_t trunc_f64(float64_t val)
{
	return float64_trunc(val);
}

static inline float32_t trunc_f32(float32_t val)
{
	return float32_trunc(val);
}

static inline float64_t cos(float64_t val)
{
	return cos_f64(val);
}

static inline float32_t cosf(float32_t val)
{
	return cos_f32(val);
}

static inline float64_t fmod(float64_t dividend, float64_t divisor)
{
	return fmod_f64(dividend, divisor);
}

static inline float32_t fmodf(float32_t dividend, float32_t divisor)
{
	return fmod_f32(dividend, divisor);
}

static inline float64_t sin(float64_t val)
{
	return sin_f64(val);
}

static inline float32_t sinf(float32_t val)
{
	return sin_f32(val);
}

static inline float64_t trunc(float64_t val)
{
	return trunc_f64(val);
}

static inline float32_t truncf(float32_t val)
{
	return trunc_f32(val);
}

#define M_LN10 2.30258509299404568402
#define M_LN2 0.69314718055994530942
#define M_LOG2E 1.4426950408889634074
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923

#endif

/** @}
 */
