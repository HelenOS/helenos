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

#include <atan2.h>
#include <errno.h>
#include <math.h>

/** Inverse tangent of two variables (32-bit floating point)
 *
 * @param y
 * @param x
 *
 * @return Inverse tangent of @a y / @a x.
 *
 */
float32_t float32_atan2(float32_t y, float32_t x)
{
	if (x >= 0)
		return atan_f32(y / x);
	else if (y >= 0)
		return M_PI - atan_f32(y / -x);
	else
		return -M_PI + atan_f32(y / -x);
}

/** Inverse tangent of two variables (64-bit floating point)
 *
 * @param y
 * @param x
 *
 * @return Inverse tangent of @a y / @a x.
 *
 */
float64_t float64_atan2(float64_t y, float64_t x)
{
	if (x >= 0)
		return atan_f64(y / x);
	else if (y >= 0)
		return M_PI - atan_f64(y / -x);
	else
		return -M_PI + atan_f64(y / -x);
}

/** @}
 */
