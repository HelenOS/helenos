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

#include <asin.h>
#include <errno.h>
#include <math.h>

/** Inverse sine (32-bit floating point)
 *
 * Compute inverse sine value.
 *
 * @param arg Inverse sine argument.
 *
 * @return Inverse sine value.
 *
 */
float32_t float32_asin(float32_t arg)
{
	float32_t aval;

	if (arg < -1.0 || arg > 1.0) {
		errno = EDOM;
		return FLOAT32_NAN;
	}

	aval = 2.0 * atan_f32(arg / (1.0 + sqrt_f32(1.0 - arg*arg)));
	if (arg > 0.0)
		return aval;
	else
		return -aval;
}

/** Inverse sine (64-bit floating point)
 *
 * Compute inverse sine value.
 *
 * @param arg Inverse sine argument.
 *
 * @return Inverse sine value.
 *
 */
float64_t float64_asin(float64_t arg)
{
	float64_t aval;

	if (arg < -1.0 || arg > 1.0) {
		errno = EDOM;
		return FLOAT64_NAN;
	}

	aval = 2.0 * atan_f64(arg / (1.0 + sqrt_f64(1.0 - arg*arg)));
	if (arg > 0.0)
		return aval;
	else
		return -aval;
}

/** @}
 */
