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

/** Cosine (32-bit floating point)
 *
 * Compute cosine value.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
float cosf(float arg)
{
	float base_arg = fmodf(arg, 2 * M_PI);

	if (base_arg < 0)
		return __math_base_cos_32(-base_arg);

	return __math_base_cos_32(base_arg);
}

/** Cosine (64-bit floating point)
 *
 * Compute cosine value.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
double cos(double arg)
{
	double base_arg = fmod(arg, 2 * M_PI);

	if (base_arg < 0)
		return __math_base_cos_64(-base_arg);

	return __math_base_cos_64(base_arg);
}

/** @}
 */
