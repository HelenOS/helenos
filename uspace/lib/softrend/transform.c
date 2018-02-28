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

#include <math.h>
#include "transform.h"

void transform_product(transform_t *res, const transform_t *a,
    const transform_t *b)
{
	for (unsigned int i = 0; i < TRANSFORM_MATRIX_DIM; i++) {
		for (unsigned int j = 0; j < TRANSFORM_MATRIX_DIM; j++) {
			double comb = 0;

			for (unsigned int k = 0; k < TRANSFORM_MATRIX_DIM; k++)
				comb += a->matrix[i][k] * b->matrix[k][j];

			res->matrix[i][j] = comb;
		}
	}
}

void transform_invert(transform_t *trans)
{
	double a = trans->matrix[1][1] * trans->matrix[2][2] -
	    trans->matrix[1][2] * trans->matrix[2][1];
	double b = trans->matrix[1][2] * trans->matrix[2][0] -
	    trans->matrix[2][2] * trans->matrix[1][0];
	double c = trans->matrix[1][0] * trans->matrix[2][1] -
	    trans->matrix[1][1] * trans->matrix[2][0];
	double d = trans->matrix[0][2] * trans->matrix[2][1] -
	    trans->matrix[0][1] * trans->matrix[2][2];
	double e = trans->matrix[0][0] * trans->matrix[2][2] -
	    trans->matrix[0][2] * trans->matrix[2][0];
	double f = trans->matrix[2][0] * trans->matrix[0][1] -
	    trans->matrix[0][0] * trans->matrix[2][1];
	double g = trans->matrix[0][1] * trans->matrix[1][2] -
	    trans->matrix[0][2] * trans->matrix[1][1];
	double h = trans->matrix[0][2] * trans->matrix[1][0] -
	    trans->matrix[0][0] * trans->matrix[1][2];
	double k = trans->matrix[0][0] * trans->matrix[1][1] -
	    trans->matrix[0][1] * trans->matrix[1][0];

	double det = 1 / (a * trans->matrix[0][0] + b * trans->matrix[0][1] +
	    c * trans->matrix[0][2]);

	trans->matrix[0][0] = a * det;
	trans->matrix[1][0] = b * det;
	trans->matrix[2][0] = c * det;

	trans->matrix[0][1] = d * det;
	trans->matrix[1][1] = e * det;
	trans->matrix[2][1] = f * det;

	trans->matrix[0][2] = g * det;
	trans->matrix[1][2] = h * det;
	trans->matrix[2][2] = k * det;
}

void transform_identity(transform_t *trans)
{
	trans->matrix[0][0] = 1;
	trans->matrix[1][0] = 0;
	trans->matrix[2][0] = 0;

	trans->matrix[0][1] = 0;
	trans->matrix[1][1] = 1;
	trans->matrix[2][1] = 0;

	trans->matrix[0][2] = 0;
	trans->matrix[1][2] = 0;
	trans->matrix[2][2] = 1;
}

void transform_translate(transform_t *trans, double dx, double dy)
{
	transform_t a;

	a.matrix[0][0] = 1;
	a.matrix[1][0] = 0;
	a.matrix[2][0] = 0;

	a.matrix[0][1] = 0;
	a.matrix[1][1] = 1;
	a.matrix[2][1] = 0;

	a.matrix[0][2] = dx;
	a.matrix[1][2] = dy;
	a.matrix[2][2] = 1;

	transform_t b = *trans;

	transform_product(trans, &a, &b);
}

void transform_scale(transform_t *trans, double qx, double qy)
{
	transform_t a;

	a.matrix[0][0] = qx;
	a.matrix[1][0] = 0;
	a.matrix[2][0] = 0;

	a.matrix[0][1] = 0;
	a.matrix[1][1] = qy;
	a.matrix[2][1] = 0;

	a.matrix[0][2] = 0;
	a.matrix[1][2] = 0;
	a.matrix[2][2] = 1;

	transform_t b = *trans;

	transform_product(trans, &a, &b);
}

void transform_rotate(transform_t *trans, double angle)
{
	transform_t a;

	a.matrix[0][0] = cos(angle);
	a.matrix[1][0] = sin(angle);
	a.matrix[2][0] = 0;

	a.matrix[0][1] = -sin(angle);
	a.matrix[1][1] = cos(angle);
	a.matrix[2][1] = 0;

	a.matrix[0][2] = 0;
	a.matrix[1][2] = 0;
	a.matrix[2][2] = 1;

	transform_t b = *trans;

	transform_product(trans, &a, &b);
}

bool transform_is_fast(transform_t *trans)
{
	return ((trans->matrix[0][0] == 1) && (trans->matrix[0][1] == 0) &&
	    (trans->matrix[1][0] == 0) && (trans->matrix[1][1] == 1) &&
	    ((trans->matrix[0][2] - trunc(trans->matrix[0][2])) == 0.0) &&
	    ((trans->matrix[1][2] - trunc(trans->matrix[1][2])) == 0.0));
}

void transform_apply_linear(const transform_t *trans, double *x, double *y)
{
	double old_x = *x;
	double old_y = *y;

	*x = old_x * trans->matrix[0][0] + old_y * trans->matrix[0][1];
	*y = old_x * trans->matrix[1][0] + old_y * trans->matrix[1][1];
}

void transform_apply_affine(const transform_t *trans, double *x, double *y)
{
	double old_x = *x;
	double old_y = *y;

	*x = old_x * trans->matrix[0][0] + old_y * trans->matrix[0][1] +
	    trans->matrix[0][2];
	*y = old_x * trans->matrix[1][0] + old_y * trans->matrix[1][1] +
	    trans->matrix[1][2];
}

/** @}
 */
