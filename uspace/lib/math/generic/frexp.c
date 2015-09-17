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

#include <frexp.h>
#include <math.h>

/** Break single precision number into fraction and exponent
 *
 * Return f and *exp such that x = f * 2^(*exp) and f is in [0.5,1)
 *
 * @param x Number
 * @param exp Place to store exponent
 *
 * @return f
 *
 */
float32_t float32_frexp(float32_t x, int *exp)
{
	float32_u u;

	u.val = x;
	*exp = u.data.parts.exp - FLOAT32_BIAS;
	u.data.parts.exp = FLOAT32_BIAS;

	return u.val;
}

/** Break double precision number into fraction and exponent
 *
 * Return f and *exp such that x = f * 2^(*exp) and f is in [0.5,1)
 *
 * @param x Number
 * @param exp Place to store exponent
 *
 * @return f
 *
 */
float64_t float64_frexp(float64_t x, int *exp)
{
	float64_u u;

	u.val = x;
	*exp = u.data.parts.exp - FLOAT64_BIAS;
	u.data.parts.exp = FLOAT64_BIAS;

	return u.val;
}

/** @}
 */
