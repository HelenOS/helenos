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

/** @addtogroup libmathsparc64
 * @{
 */
/** @file
 */

#ifndef LIBMATH_sparc64_MATH_H_
#define LIBMATH_sparc64_MATH_H_

#include <acos.h>
#include <asin.h>
#include <atan.h>
#include <atan2.h>
#include <ceil.h>
#include <cosh.h>
#include <exp.h>
#include <fabs.h>
#include <floor.h>
#include <fmod.h>
#include <frexp.h>
#include <ldexp.h>
#include <log.h>
#include <log10.h>
#include <mathtypes.h>
#include <modf.h>
#include <pow.h>
#include <sinh.h>
#include <sqrt.h>
#include <tan.h>
#include <tanh.h>
#include <trig.h>
#include <trunc.h>

#define HUGE_VAL FLOAT64_INF

static inline float64_t acos_f64(float64_t val)
{
	return float64_acos(val);
}

static inline float32_t acos_f32(float32_t val)
{
	return float32_acos(val);
}

static inline float64_t asin_f64(float64_t val)
{
	return float64_asin(val);
}

static inline float32_t asin_f32(float32_t val)
{
	return float32_asin(val);
}

static inline float64_t atan_f64(float64_t val)
{
	return float64_atan(val);
}

static inline float32_t atan_f32(float32_t val)
{
	return float32_atan(val);
}

static inline float64_t atan2_f64(float64_t y, float64_t x)
{
	return float64_atan2(y, x);
}

static inline float32_t atan2_f32(float32_t y, float32_t x)
{
	return float32_atan2(y, x);
}

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

static inline float64_t cosh_f64(float64_t val)
{
	return float64_cosh(val);
}

static inline float32_t cosh_f32(float32_t val)
{
	return float32_cosh(val);
}

static inline float64_t exp_f64(float64_t val)
{
	return float64_exp(val);
}

static inline float32_t exp_f32(float32_t val)
{
	return float32_exp(val);
}

static inline float64_t fabs_f64(float64_t val)
{
	return float64_fabs(val);
}

static inline float32_t fabs_f32(float32_t val)
{
	return float32_fabs(val);
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
	return float64_fmod(dividend, divisor);
}

static inline float64_t fmod_f32(float32_t dividend, float32_t divisor)
{
	return float32_fmod(dividend, divisor);
}

static inline float64_t frexp_f64(float64_t x, int *exp)
{
	return float64_frexp(x, exp);
}

static inline float64_t frexp_f32(float32_t x, int *exp)
{
	return float32_frexp(x, exp);
}

static inline float64_t ldexp_f64(float64_t x, int exp)
{
	return float64_ldexp(x, exp);
}

static inline float64_t ldexp_f32(float32_t x, int exp)
{
	return float32_ldexp(x, exp);
}

static inline float64_t log_f64(float64_t val)
{
	return float64_log(val);
}

static inline float32_t log_f32(float32_t val)
{
	return float32_log(val);
}

static inline float64_t log10_f64(float64_t val)
{
	return float64_log10(val);
}

static inline float32_t log10_f32(float32_t val)
{
	return float32_log10(val);
}

static inline float64_t modf_f64(float64_t value, float64_t *iptr)
{
	return float64_modf(value, iptr);
}

static inline float64_t modf_f32(float32_t value, float32_t *iptr)
{
	return float32_modf(value, iptr);
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

static inline float64_t sinh_f64(float64_t val)
{
	return float64_sinh(val);
}

static inline float32_t sinh_f32(float32_t val)
{
	return float32_sinh(val);
}

static inline float64_t sqrt_f64(float64_t val)
{
	return float64_sqrt(val);
}

static inline float32_t sqrt_f32(float32_t val)
{
	return float32_sqrt(val);
}

static inline float64_t tan_f64(float64_t val)
{
	return float64_tan(val);
}

static inline float32_t tan_f32(float32_t val)
{
	return float32_tan(val);
}

static inline float64_t tanh_f64(float64_t val)
{
	return float64_tanh(val);
}

static inline float32_t tanh_f32(float32_t val)
{
	return float32_tanh(val);
}

static inline float64_t trunc_f64(float64_t val)
{
	return float64_trunc(val);
}

static inline float32_t trunc_f32(float32_t val)
{
	return float32_trunc(val);
}

static inline float64_t acos(float64_t val)
{
	return acos_f64(val);
}

static inline float32_t acosf(float32_t val)
{
	return acos_f32(val);
}

static inline float64_t asin(float64_t val)
{
	return asin_f64(val);
}

static inline float32_t asinf(float32_t val)
{
	return asin_f32(val);
}

static inline float64_t atan(float64_t val)
{
	return atan_f64(val);
}

static inline float32_t atanf(float32_t val)
{
	return atan_f32(val);
}

static inline float64_t atan2(float64_t y, float64_t x)
{
	return atan2_f64(y, x);
}

static inline float32_t atan2f(float32_t y, float32_t x)
{
	return atan2_f32(y, x);
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

static inline float64_t cosh(float64_t val)
{
	return cosh_f64(val);
}

static inline float32_t coshf(float32_t val)
{
	return cosh_f32(val);
}

static inline float64_t exp(float64_t val)
{
	return exp_f64(val);
}

static inline float32_t expf(float32_t val)
{
	return exp_f32(val);
}

static inline float64_t fabs(float64_t val)
{
	return fabs_f64(val);
}

static inline float32_t fabsf(float32_t val)
{
	return fabs_f32(val);
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

static inline float64_t frexp(float64_t x, int *exp)
{
	return frexp_f64(x, exp);
}

static inline float32_t frexpf(float32_t x, int *exp)
{
	return frexp_f32(x, exp);
}

static inline float64_t ldexp(float64_t x, int exp)
{
	return ldexp_f64(x, exp);
}

static inline float32_t ldexpf(float32_t x, int exp)
{
	return ldexp_f32(x, exp);
}

static inline float64_t log(float64_t val)
{
	return log_f64(val);
}

static inline float32_t logf(float32_t val)
{
	return log_f32(val);
}

static inline float64_t log10(float64_t val)
{
	return log10_f64(val);
}

static inline float32_t log10f(float32_t val)
{
	return log10_f32(val);
}

static inline float64_t modf(float64_t value, float64_t *iptr)
{
	return modf_f64(value, iptr);
}

static inline float32_t modff(float32_t value, float32_t *iptr)
{
	return modf_f32(value, iptr);
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

static inline float64_t sinh(float64_t val)
{
	return sinh_f64(val);
}

static inline float32_t sinhf(float32_t val)
{
	return sinh_f32(val);
}

static inline float64_t sqrt(float64_t val)
{
	return sqrt_f64(val);
}

static inline float32_t sqrtf(float32_t val)
{
	return sqrt_f32(val);
}

static inline float64_t tan(float64_t val)
{
	return tan_f64(val);
}

static inline float32_t tanf(float32_t val)
{
	return tan_f32(val);
}

static inline float64_t tanh(float64_t val)
{
	return tanh_f64(val);
}

static inline float32_t tanhf(float32_t val)
{
	return tanh_f32(val);
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
