/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup libmathsparc32
 * @{
 */
/** @file
 */

#ifndef LIBMATH_sparc32_MATH_H_
#define LIBMATH_sparc32_MATH_H_

#include <ceil.h>
#include <exp.h>
#include <floor.h>
#include <log.h>
#include <mathtypes.h>
#include <mod.h>
#include <pow.h>
#include <trunc.h>
#include <trig.h>

static inline float64_t ceil_f64(float64_t val)
{
	return float64_ceil(val);
}

static inline float32_t ceil_f32(float32_t val)
{
	return float32_ceil(val);
}

static inline float64_t cos_f64(float64_t val)
{
	return float64_cos(val);
}

static inline float32_t cos_f32(float32_t val)
{
	return float32_cos(val);
}

static inline float64_t exp_f64(float64_t val)
{
	return float64_exp(val);
}

static inline float32_t exp_f32(float32_t val)
{
	return float32_exp(val);
}

static inline float64_t floor_f64(float64_t val)
{
	return float64_floor(val);
}

static inline float32_t floor_f32(float32_t val)
{
	return float32_floor(val);
}

static inline float64_t fmod_f64(float64_t dividend, float64_t divisor)
{
	return float64_mod(dividend, divisor);
}

static inline float64_t fmod_f32(float32_t dividend, float32_t divisor)
{
	return float32_mod(dividend, divisor);
}

static inline float64_t log_f64(float64_t val)
{
	return float64_log(val);
}

static inline float32_t log_f32(float32_t val)
{
	return float32_log(val);
}

static inline float64_t pow_f64(float64_t x, float64_t y)
{
	return float64_pow(x, y);
}

static inline float32_t pow_f32(float32_t x, float32_t y)
{
	return float32_pow(x, y);
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

static inline float64_t ceil(float64_t val)
{
	return ceil_f64(val);
}

static inline float32_t ceilf(float32_t val)
{
	return ceil_f32(val);
}

static inline float64_t cos(float64_t val)
{
	return cos_f64(val);
}

static inline float32_t cosf(float32_t val)
{
	return cos_f32(val);
}

static inline float64_t exp(float64_t val)
{
	return exp_f64(val);
}

static inline float32_t expf(float32_t val)
{
	return exp_f32(val);
}

static inline float64_t floor(float64_t val)
{
	return floor_f64(val);
}

static inline float32_t floorf(float32_t val)
{
	return floor_f32(val);
}

static inline float64_t fmod(float64_t dividend, float64_t divisor)
{
	return fmod_f64(dividend, divisor);
}

static inline float32_t fmodf(float32_t dividend, float32_t divisor)
{
	return fmod_f32(dividend, divisor);
}

static inline float64_t log(float64_t val)
{
	return log_f64(val);
}

static inline float32_t logf(float32_t val)
{
	return log_f32(val);
}

static inline float64_t pow(float64_t x, float64_t y)
{
	return pow_f64(x, y);
}

static inline float32_t powf(float32_t x, float32_t y)
{
	return pow_f32(x, y);
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

#endif

/** @}
 */
