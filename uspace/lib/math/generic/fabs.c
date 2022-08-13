/*
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

float fabsf(float val)
{
	return copysignf(val, 1.0f);
}

double fabs(double val)
{
	return copysign(val, 1.0);
}

/** @}
 */
