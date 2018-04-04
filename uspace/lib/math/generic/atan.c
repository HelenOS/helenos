/*
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
/** @file
 */

#include <atan.h>
#include <errno.h>
#include <math.h>

#define SERIES_DEGREE_32 13
#define SERIES_DEGREE_64 33

/** Inverse tangent approximation by Euler's series (32-bit floating point)
 *
 * Compute the approximation of inverse tangent by a
 * series found by Leonhard Euler (using the first SERIES_DEGREE terms).
 *
 * @param arg Inverse tangent argument.
 *
 * @return Inverse tangent value approximation.
 *
 */
static float32_t series_atan_32(float32_t arg)
{
	float32_t sum = 0;
	float32_t a = arg / (1.0 + arg * arg);

	/*
	 * atan(z) = sum(n=0, +inf) [ (2^2n) * (n!)^2 / (2n + 1)! *
	 *    z^(2n+1) / (1 + z^2)^(n+1) ]
	 */

	for (unsigned int n = 0; n < SERIES_DEGREE_32; n++) {
		if (n > 0) {
			a = a * n * n;
			a = a / (2.0 * n + 1.0) / (2.0 * n);
		}
		sum += a;
		a = a * 4.0 * arg * arg / (1.0 + arg * arg);
	}

	return sum;
}

/** Inverse tangent approximation by Euler's series (64-bit floating point)
 *
 * Compute the approximation of inverse tangent by a
 * series found by Leonhard Euler (using the first SERIES_DEGREE terms).
 *
 * @param arg Inverse tangent argument.
 *
 * @return Inverse tangent value approximation.
 *
 */
static float64_t series_atan_64(float64_t arg)
{
	float64_t sum = 0;
	float64_t a = arg / (1.0 + arg * arg);

	/*
	 * atan(z) = sum(n=0, +inf) [ (2^2n) * (n!)^2 / (2n + 1)! *
	 *    z^(2n+1) / (1 + z^2)^(n+1) ]
	 */

	for (unsigned int n = 0; n < SERIES_DEGREE_64; n++) {
		if (n > 0) {
			a = a * n * n;
			a = a / (2.0 * n + 1.0) / (2.0 * n);
		}
		sum += a;
		a = a * 4.0 * arg * arg / (1.0 + arg * arg);
	}

	return sum;
}

/** Inverse tangent (32-bit floating point)
 *
 * Compute inverse sine value.
 *
 * @param arg Inverse sine argument.
 *
 * @return Inverse sine value.
 *
 */
float32_t float32_atan(float32_t arg)
{
	if (arg < -1.0 || arg > 1.0)
		return 2.0 * series_atan_32(arg / (1.0 + sqrt_f32(1.0 + arg * arg)));
	else
		return series_atan_32(arg);
}

/** Inverse tangent (64-bit floating point)
 *
 * Compute inverse sine value.
 *
 * @param arg Inverse sine argument.
 *
 * @return Inverse sine value.
 *
 */
float64_t float64_atan(float64_t arg)
{
	if (arg < -1.0 || arg > 1.0)
		return 2.0 * series_atan_64(arg / (1.0 + sqrt_f64(1.0 + arg * arg)));
	else
		return series_atan_64(arg);
}

/** @}
 */
