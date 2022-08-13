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

/** Sine (32-bit floating point)
 *
 * Compute sine value.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
float sinf(float arg)
{
	float base_arg = fmodf(arg, 2 * M_PI);

	if (base_arg < 0)
		return -__math_base_sin_32(-base_arg);

	return __math_base_sin_32(base_arg);
}

/** Sine (64-bit floating point)
 *
 * Compute sine value.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
double sin(double arg)
{
	double base_arg = fmod(arg, 2 * M_PI);

	if (base_arg < 0)
		return -__math_base_sin_64(-base_arg);

	return __math_base_sin_64(base_arg);
}

/** @}
 */
