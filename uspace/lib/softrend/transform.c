/*
 * Copyright (c) 2011 Petr Koupy
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

#include "transform.h"

void transform_multiply(transform_t *res, const transform_t *left, const transform_t *right)
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			double comb = 0;
			for (int k = 0; k < 3; ++k) {
				comb += left->m[i][k] * right->m[k][j];
			}
			res->m[i][j] = comb;
		}
	}
}

void transform_invert(transform_t *t)
{
	double a = t->m[1][1] * t->m[2][2] - t->m[1][2] * t->m[2][1];
	double b = t->m[1][2] * t->m[2][0] - t->m[2][2] * t->m[1][0];
	double c = t->m[1][0] * t->m[2][1] - t->m[1][1] * t->m[2][0];
	double d = t->m[0][2] * t->m[2][1] - t->m[0][1] * t->m[2][2];
	double e = t->m[0][0] * t->m[2][2] - t->m[0][2] * t->m[2][0];
	double f = t->m[2][0] * t->m[0][1] - t->m[0][0] * t->m[2][1];
	double g = t->m[0][1] * t->m[1][2] - t->m[0][2] * t->m[1][1];
	double h = t->m[0][2] * t->m[1][0] - t->m[0][0] * t->m[1][2];
	double k = t->m[0][0] * t->m[1][1] - t->m[0][1] * t->m[1][0];

	double det = t->m[0][0] * a + t->m[0][1] * b + t->m[0][2] * c;
	det = 1 / det;

	t->m[0][0] = a * det;   t->m[0][1] = d * det;   t->m[0][2] = g * det;
	t->m[1][0] = b * det;   t->m[1][1] = e * det;   t->m[1][2] = h * det;
	t->m[2][0] = c * det;   t->m[2][1] = f * det;   t->m[2][2] = k * det;
}

void transform_identity(transform_t *t)
{
	t->m[0][0] = 1;   t->m[0][1] = 0;   t->m[0][2] = 0;
	t->m[1][0] = 0;   t->m[1][1] = 1;   t->m[1][2] = 0;
	t->m[2][0] = 0;   t->m[2][1] = 0;   t->m[2][2] = 1;
}

void transform_translate(transform_t *t, double dx, double dy)
{
	transform_t a;
	a.m[0][0] = 1;   a.m[0][1] = 0;   a.m[0][2] = dx;
	a.m[1][0] = 0;   a.m[1][1] = 1;   a.m[1][2] = dy;
	a.m[2][0] = 0;   a.m[2][1] = 0;   a.m[2][2] = 1;

	transform_t r;
	transform_multiply(&r, &a, t);
	*t = r;
}

void transform_scale(transform_t *t, double qx, double qy)
{
	transform_t a;
	a.m[0][0] = qx;  a.m[0][1] = 0;   a.m[0][2] = 0;
	a.m[1][0] = 0;   a.m[1][1] = qy;  a.m[1][2] = 0;
	a.m[2][0] = 0;   a.m[2][1] = 0;   a.m[2][2] = 1;

	transform_t r;
	transform_multiply(&r, &a, t);
	*t = r;
}

void transform_rotate(transform_t *t, double rad)
{
	double s, c;

	// FIXME: temporary solution until there are trigonometric functions in libc

	while (rad < 0) {
		rad += (2 * PI);
	}
	
	while (rad > (2 * PI)) {
		rad -= (2 * PI);
	}

	if (rad >= 0 && rad < (PI / 4)) {
		s = 0; c = 1;
	} else if (rad >= (PI / 4) && rad < (3 * PI / 4)) {
		s = 1; c = 0;
	} else if (rad >= (3 * PI / 4) && rad < (5 * PI / 4)) {
		s = 0; c = -1;
	} else if (rad >= (5 * PI / 4) && rad < (7 * PI / 4)) {
		s = -1; c = 0;
	} else {
		s = 0; c = 1;
	}

	transform_t a;
	a.m[0][0] = c;   a.m[0][1] = -s;  a.m[0][2] = 0;
	a.m[1][0] = s;   a.m[1][1] = c;   a.m[1][2] = 0;
	a.m[2][0] = 0;   a.m[2][1] = 0;   a.m[2][2] = 1;

	transform_t r;
	transform_multiply(&r, &a, t);
	*t = r;
}

void transform_apply_linear(const transform_t *t, double *x, double *y)
{
	double x_ = *x;
	double y_ = *y;
	*x = x_ * t->m[0][0] + y_ * t->m[0][1];
	*y = x_ * t->m[1][0] + y_ * t->m[1][1];
}

void transform_apply_affine(const transform_t *t, double *x, double *y)
{
	double x_ = *x;
	double y_ = *y;
	*x = x_ * t->m[0][0] + y_ * t->m[0][1] + t->m[0][2];
	*y = x_ * t->m[1][0] + y_ * t->m[1][1] + t->m[1][2];
}

/** @}
 */
