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
	uint8_t res_a;
	uint8_t res_r;
	uint8_t res_g;
	uint8_t res_b;
	uint32_t res_a_inv;

	res_a_inv = ALPHA(bg) * (255 - ALPHA(fg));
	res_a = ALPHA(fg) + (res_a_inv / 255);

	res_r = (RED(fg) * ALPHA(fg) / 255) + (RED(bg) * res_a_inv) / (255 * 255);
	res_g = (GREEN(fg) * ALPHA(fg) / 255) + (GREEN(bg) * res_a_inv) / (255 * 255);
	res_b = (BLUE(fg) * ALPHA(fg) / 255) + (BLUE(bg) * res_a_inv) / (255 * 255);

	return PIXEL(res_a, res_r, res_g, res_b);
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
