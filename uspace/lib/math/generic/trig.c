/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libmath
 * @{
 */
/** @file
 */

#include <math.h>
#include <trig.h>

#define TAYLOR_DEGREE_32 13
#define TAYLOR_DEGREE_64 21

/** Precomputed values for factorial (starting from 1!) */
static float64_t factorials[TAYLOR_DEGREE_64] = {
	1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800,
	479001600, 6227020800.0L, 87178291200.0L, 1307674368000.0L,
	20922789888000.0L, 355687428096000.0L, 6402373705728000.0L,
	121645100408832000.0L, 2432902008176640000.0L, 51090942171709440000.0L
};

/** Sine approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of sine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Sine argument.
 *
 * @return Sine value approximation.
 *
 */
static float32_t taylor_sin_32(float32_t arg)
{
	float32_t ret = 0;
	float32_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_32; i++) {
		nom *= arg;

		if ((i % 4) == 0)
			ret += nom / factorials[i];
		else if ((i % 4) == 2)
			ret -= nom / factorials[i];
	}

	return ret;
}

/** Sine approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of sine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Sine argument.
 *
 * @return Sine value approximation.
 *
 */
static float64_t taylor_sin_64(float64_t arg)
{
	float64_t ret = 0;
	float64_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_64; i++) {
		nom *= arg;

		if ((i % 4) == 0)
			ret += nom / factorials[i];
		else if ((i % 4) == 2)
			ret -= nom / factorials[i];
	}

	return ret;
}

/** Cosine approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of cosine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value approximation.
 *
 */
static float32_t taylor_cos_32(float32_t arg)
{
	float32_t ret = 1;
	float32_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_32; i++) {
		nom *= arg;

		if ((i % 4) == 1)
			ret -= nom / factorials[i];
		else if ((i % 4) == 3)
			ret += nom / factorials[i];
	}

	return ret;
}

/** Cosine approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of cosine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value approximation.
 *
 */
static float64_t taylor_cos_64(float64_t arg)
{
	float64_t ret = 1;
	float64_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_64; i++) {
		nom *= arg;

		if ((i % 4) == 1)
			ret -= nom / factorials[i];
		else if ((i % 4) == 3)
			ret += nom / factorials[i];
	}

	return ret;
}

/** Sine value for values within base period (32-bit floating point)
 *
 * Compute the value of sine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
static float32_t base_sin_32(float32_t arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_sin_32(arg);
	case 1:
	case 2:
		return taylor_cos_32(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_sin_32(arg - M_PI);
	case 5:
	case 6:
		return -taylor_cos_32(arg - 3 * M_PI / 2);
	default:
		return taylor_sin_32(arg - 2 * M_PI);
	}
}

/** Sine value for values within base period (64-bit floating point)
 *
 * Compute the value of sine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
static float64_t base_sin_64(float64_t arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_sin_64(arg);
	case 1:
	case 2:
		return taylor_cos_64(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_sin_64(arg - M_PI);
	case 5:
	case 6:
		return -taylor_cos_64(arg - 3 * M_PI / 2);
	default:
		return taylor_sin_64(arg - 2 * M_PI);
	}
}

/** Cosine value for values within base period (32-bit floating point)
 *
 * Compute the value of cosine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
static float32_t base_cos_32(float32_t arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_cos_32(arg);
	case 1:
	case 2:
		return -taylor_sin_32(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_cos_32(arg - M_PI);
	case 5:
	case 6:
		return taylor_sin_32(arg - 3 * M_PI / 2);
	default:
		return taylor_cos_32(arg - 2 * M_PI);
	}
}

/** Cosine value for values within base period (64-bit floating point)
 *
 * Compute the value of cosine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
static float64_t base_cos_64(float64_t arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_cos_64(arg);
	case 1:
	case 2:
		return -taylor_sin_64(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_cos_64(arg - M_PI);
	case 5:
	case 6:
		return taylor_sin_64(arg - 3 * M_PI / 2);
	default:
		return taylor_cos_64(arg - 2 * M_PI);
	}
}

/** Sine (32-bit floating point)
 *
 * Compute sine value.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
float32_t float32_sin(float32_t arg)
{
	float32_t base_arg = fmod_f32(arg, 2 * M_PI);

	if (base_arg < 0)
		return -base_sin_32(-base_arg);

	return base_sin_32(base_arg);
}

/** Sine (64-bit floating point)
 *
 * Compute sine value.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
float64_t float64_sin(float64_t arg)
{
	float64_t base_arg = fmod_f64(arg, 2 * M_PI);

	if (base_arg < 0)
		return -base_sin_64(-base_arg);

	return base_sin_64(base_arg);
}

/** Cosine (32-bit floating point)
 *
 * Compute cosine value.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
float32_t float32_cos(float32_t arg)
{
	float32_t base_arg = fmod_f32(arg, 2 * M_PI);

	if (base_arg < 0)
		return base_cos_32(-base_arg);

	return base_cos_32(base_arg);
}

/** Cosine (64-bit floating point)
 *
 * Compute cosine value.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
float64_t float64_cos(float64_t arg)
{
	float64_t base_arg = fmod_f64(arg, 2 * M_PI);

	if (base_arg < 0)
		return base_cos_64(-base_arg);

	return base_cos_64(base_arg);
}

/** @}
 */
