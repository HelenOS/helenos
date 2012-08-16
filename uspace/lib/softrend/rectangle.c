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

#include "rectangle.h"

bool rectangle_intersect(
    sysarg_t x1, sysarg_t y1, sysarg_t w1, sysarg_t h1,
    sysarg_t x2, sysarg_t y2, sysarg_t w2, sysarg_t h2,
    sysarg_t *x_out, sysarg_t *y_out, sysarg_t *w_out, sysarg_t *h_out)
{
	sysarg_t l, r, t, b;

	l = x1 < x2 ? x2 : x1;
	t = y1 < y2 ? y2 : y1;
	r = x1 + w1 < x2 + w2 ? x1 + w1 : x2 + w2;
	b = y1 + h1 < y2 + h2 ? y1 + h1 : y2 + h2;

	if (l < r && t < b) {
		(*x_out) = l;
		(*y_out) = t;
		(*w_out) = r - l;
		(*h_out) = b - t;
		return true;
	} else {
		(*x_out) = 0;
		(*y_out) = 0;
		(*w_out) = 0;
		(*h_out) = 0;
		return false;
	}
}

void rectangle_union(
    sysarg_t x1, sysarg_t y1, sysarg_t w1, sysarg_t h1,
    sysarg_t x2, sysarg_t y2, sysarg_t w2, sysarg_t h2,
    sysarg_t *x_out, sysarg_t *y_out, sysarg_t *w_out, sysarg_t *h_out)
{
	sysarg_t l, r, t, b;

	l = x1 > x2 ? x2 : x1;
	t = y1 > y2 ? y2 : y1;
	r = x1 + w1 > x2 + w2 ? x1 + w1 : x2 + w2;
	b = y1 + h1 > y2 + h2 ? y1 + h1 : y2 + h2;

	(*x_out) = l;
	(*y_out) = t;
	(*w_out) = r - l;
	(*h_out) = b - t;
}

/** @}
 */
