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

#include <exp.h>
#include <math.h>

#define TAYLOR_DEGREE_32 13
#define TAYLOR_DEGREE_64 21

/** Precomputed values for factorial (starting from 1!) */
static float64_t factorials[TAYLOR_DEGREE_64] = {
	1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800,
	479001600, 6227020800.0L, 87178291200.0L, 1307674368000.0L,
	20922789888000.0L, 355687428096000.0L, 6402373705728000.0L,
	121645100408832000.0L, 2432902008176640000.0L, 51090942171709440000.0L
};

/** Exponential approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of exponential by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval XXXX.
 *
 * @param arg Argument.
 *
 * @return Exponential value approximation.
 *
 */
static float32_t taylor_exp_32(float32_t arg)
{
	float32_t ret = 1;
	float32_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_32; i++) {
		nom *= arg;
		ret += nom / factorials[i];
	}

	return ret;
}

/** Exponential approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of exponential by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval XXXX.
 *
 * @param arg Argument.
 *
 * @return Exponential value approximation.
 *
 */
static float64_t taylor_exp_64(float64_t arg)
{
	float64_t ret = 1;
	float64_t nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_64; i++) {
		nom *= arg;
		ret += nom / factorials[i];
	}

	return ret;
}

/** Exponential (32-bit floating point)
 *
 * Compute exponential value.
 *
 * @param arg Exponential argument.
 *
 * @return Exponential value.
 *
 */
float32_t float32_exp(float32_t arg)
{
	float32_t f;
	float32_t i;
	float32_u r;

	/*
	 * e^a = (2 ^ log2(e))^a = 2 ^ (log2(e) * a)
	 * log2(e) * a = i + f | f in [0, 1]
	 * e ^ a = 2 ^ (i + f) = 2^f * 2^i = (e ^ log(2))^f * 2^i =
	 * e^(log(2)*f) * 2^i
	 */

	i = trunc_f32(arg * M_LOG2E);
	f = arg * M_LOG2E - i;

	r.val = taylor_exp_32(M_LN2 * f);
	r.data.parts.exp += i;
	return r.val;
}

/** Exponential (64-bit floating point)
 *
 * Compute exponential value.
 *
 * @param arg Exponential argument.
 *
 * @return Exponential value.
 *
 */
float64_t float64_exp(float64_t arg)
{
	float64_t f;
	float64_t i;
	float64_u r;

	/*
	 * e^a = (2 ^ log2(e))^a = 2 ^ (log2(e) * a)
	 * log2(e) * a = i + f | f in [0, 1]
	 * e ^ a = 2 ^ (i + f) = 2^f * 2^i = (e ^ log(2))^f * 2^i =
	 * e^(log(2)*f) * 2^i
	 */

	i = trunc_f64(arg * M_LOG2E);
	f = arg * M_LOG2E - i;

	r.val = taylor_exp_64(M_LN2 * f);
	r.data.parts.exp += i;
	return r.val;
}

/** @}
 */
