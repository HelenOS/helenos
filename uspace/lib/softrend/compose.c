/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup softrend
 * @{
 */
/**
 * @file
 */

#include "compose.h"

pixel_t compose_clr(pixel_t fg, pixel_t bg)
{
	return 0;
}

pixel_t compose_src(pixel_t fg, pixel_t bg)
{
	return fg;
}

pixel_t compose_dst(pixel_t fg, pixel_t bg)
{
	return bg;
}

pixel_t compose_over(pixel_t fg, pixel_t bg)
{
	double mul;
	double mul_cmp;

	double res_a;
	double res_r;
	double res_g;
	double res_b;

	if (ALPHA(bg) == 255) {
		res_a = 1;
		mul = ((double) ALPHA(fg)) / 255.0;
		mul_cmp = 1 - mul;
	} else {
		double fg_a = ((double) ALPHA(fg)) / 255.0;
		double bg_a = ((double) ALPHA(bg)) / 255.0;

		res_a = 1 - (1 - fg_a) * (1 - bg_a);
		mul = fg_a / res_a;
		mul_cmp = 1 - mul;
	}

	res_r = mul * ((double) RED(fg)) + mul_cmp * ((double) RED(bg));
	res_g = mul * ((double) GREEN(fg)) + mul_cmp * ((double) GREEN(bg));
	res_b = mul * ((double) BLUE(fg)) + mul_cmp * ((double) BLUE(bg));

	return PIXEL((unsigned) (res_a * 255),
	    (unsigned) res_r, (unsigned) res_g, (unsigned) res_b);
}

pixel_t compose_in(pixel_t fg, pixel_t bg)
{
	// TODO
	return 0;
}

pixel_t compose_out(pixel_t fg, pixel_t bg)
{
	// TODO
	return 0;
}

pixel_t compose_atop(pixel_t fg, pixel_t bg)
{
	// TODO
	return 0;
}

pixel_t compose_xor(pixel_t fg, pixel_t bg)
{
	// TODO
	return 0;
}

pixel_t compose_add(pixel_t fg, pixel_t bg)
{
	// TODO
	return 0;
}

/** @}
 */
