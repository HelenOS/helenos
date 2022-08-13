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

/** Remainder function (32-bit floating point)
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
float fmodf(float dividend, float divisor)
{
	// FIXME: replace with exact arithmetics

	float quotient = truncf(dividend / divisor);

	return (dividend - quotient * divisor);
}

/** @}
 */
