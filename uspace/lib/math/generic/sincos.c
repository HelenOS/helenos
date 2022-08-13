/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
