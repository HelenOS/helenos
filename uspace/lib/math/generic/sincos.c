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
#include "internal.h"

/**
 * Computes sine and cosine at the same time, which might be more efficient than
 * computing each separately.
 *
 * @param x  Input value.
 * @param s  Output sine value, *s = sinf(x).
 * @param c  Output cosine value, *c = cosf(x).
 */
void sincosf(float x, float *s, float *c)
{
	float base_arg = fmodf(x, 2 * M_PI);

	if (base_arg < 0) {
		*s = -__math_base_sin_32(-base_arg);
		*c = __math_base_cos_32(-base_arg);
	} else {
		*s = __math_base_sin_32(base_arg);
		*c = __math_base_cos_32(base_arg);
	}
}

/**
 * Computes sine and cosine at the same time, which might be more efficient than
 * computing each separately.
 *
 * @param x  Input value.
 * @param s  Output sine value, *s = sin(x).
 * @param c  Output cosine value, *c = cos(x).
 */
void sincos(double x, double *s, double *c)
{
	double base_arg = fmod(x, 2 * M_PI);

	if (base_arg < 0) {
		*s = -__math_base_sin_64(-base_arg);
		*c = __math_base_cos_64(-base_arg);
	} else {
		*s = __math_base_sin_64(base_arg);
		*c = __math_base_cos_64(base_arg);
	}
}

/** @}
 */
