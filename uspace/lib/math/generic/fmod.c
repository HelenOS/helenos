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

/** Remainder function (64-bit floating point)
 *
 * Calculate the modulo of dividend by divisor.
 *
 * This is a very basic implementation that uses
 * division and multiplication (instead of exact
 * arithmetics). Thus the result might be very
 * imprecise (depending on the magnitude of the
 * arguments).
 *
 * @param dividend Dividend.
 * @param divisor  Divisor.
 *
 * @return Modulo.
 *
 */
double fmod(double dividend, double divisor)
{
	// FIXME: replace with exact arithmetics

	double quotient = trunc(dividend / divisor);

	return (dividend - quotient * divisor);
}

/** @}
 */
