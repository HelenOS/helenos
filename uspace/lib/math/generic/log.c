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

#include <log.h>
#include <math.h>

#define TAYLOR_DEGREE_32  31
#define TAYLOR_DEGREE_64  63

/** log(1 - arg) approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of log(1 - arg) by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * arg must be within [-1, 1].
 *
 * @param arg Argument.
 *
 * @return log(1 - arg)
 *
 */
static float32_t taylor_log_32(float32_t arg)
{
	float32_t ret = 0;
	float32_t num = 1;

	for (unsigned int i = 1; i <= TAYLOR_DEGREE_32; i++) {
		num *= arg;

		if ((i % 2) == 0)
			ret += num / i;
		else
			ret -= num / i;
	}

	return ret;
}

/** log(1 - arg) approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of log(1 - arg) by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * arg must be within [-1, 1].
 *
 * @param arg Argument.
 *
 * @return log(1 - arg)
 *
 */
static float64_t taylor_log_64(float64_t arg)
{
	float64_t ret = 0;
	float64_t num = 1;

	for (unsigned int i = 1; i <= TAYLOR_DEGREE_64; i++) {
		num *= arg;

		if ((i % 2) == 0)
			ret += num / i;
		else
			ret -= num / i;
	}

	return ret;
}

/** Natural logarithm (32-bit floating point)
 *
 * @param arg Argument.
 *
 * @return Logarithm.
 *
 */
float32_t float32_log(float32_t arg)
{
	float32_u m;
	int e;

	m.val = arg;
	/*
	 * Factor arg into m * 2^e where m has exponent -1,
	 * which means it is in [1.0000..e-1, 1.1111..e-1] = [0.5, 1.0]
	 * so the argument to taylor_log_32 will be in [0, 0.5]
	 * ensuring that we get at least one extra bit of precision
	 * in each iteration.
	 */
	e = m.data.parts.exp - (FLOAT32_BIAS - 1);
	m.data.parts.exp = FLOAT32_BIAS - 1;

	/*
	 * arg = m * 2^e ; log(arg) = log(m) + log(2^e) =
	 * log(m) + log2(2^e) / log2(e) = log(m) + e / log2(e)
	 */
	return - taylor_log_32(m.val - 1.0) + e / M_LOG2E;
}

/** Natural logarithm (64-bit floating point)
 *
 * @param arg Argument.
 *
 * @return Logarithm.
 *
 */
float64_t float64_log(float64_t arg)
{
	float64_u m;
	int e;

	m.val = arg;

	/*
	 * Factor arg into m * 2^e where m has exponent -1,
	 * which means it is in [1.0000..e-1, 1.1111..e-1] = [0.5, 1.0]
	 * so the argument to taylor_log_32 will be in [0, 0.5]
	 * ensuring that we get at least one extra bit of precision
	 * in each iteration.
	 */
	e = m.data.parts.exp - (FLOAT64_BIAS - 1);
	m.data.parts.exp = FLOAT64_BIAS - 1;

	/*
	 * arg = m * 2^e ; log(arg) = log(m) + log(2^e) =
	 * log(m) + log2(2^e) / log2(e) = log(m) + e / log2(e)
	 */
	return - taylor_log_64(m.val - 1.0) + e / M_LOG2E;
}

/** @}
 */
