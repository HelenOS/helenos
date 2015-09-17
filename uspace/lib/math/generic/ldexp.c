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

#include <errno.h>
#include <ldexp.h>
#include <math.h>

/** Single precision multiply by power of two
 *
 * Compute x * 2^exp.
 *
 * @param x Number
 * @param exp Exponent
 *
 * @return x * 2^exp
 *
 */
float32_t float32_ldexp(float32_t x, int exp)
{
	float32_u u;
	int e;

	u.val = x;
	e = u.data.parts.exp + exp;

	if (e < 0) {
		/* XXX Can we return denormalized numbers? */
		return 0.0;
	} else if (e > FLOAT32_MAX_EXPONENT) {
		errno = ERANGE;
		if (e < 0)
			return -FLOAT32_INF;
		else
			return FLOAT32_INF;
	} else {
		/* Adjust exponent */
		u.data.parts.exp = e;
		return u.val;
	}
}

/** Double precision multiply by power of two
 *
 * Compute x * 2^exp.
 *
 * @param x Number
 * @param y Exponent
 *
 * @return x * 2^exp
 *
 */
float64_t float64_ldexp(float64_t x, int exp)
{
	float64_u u;
	int e;

	u.val = x;
	e = u.data.parts.exp + exp;

	if (e < 0) {
		/* XXX Can we return denormalized numbers? */
		return 0.0;
	} else if (e > FLOAT64_MAX_EXPONENT) {
		errno = ERANGE;
		if (e < 0)
			return -FLOAT64_INF;
		else
			return FLOAT64_INF;
	} else {
		/* Adjust exponent */
		u.data.parts.exp = e;
		return u.val;
	}
}

/** @}
 */
