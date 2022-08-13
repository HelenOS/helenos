/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <assert.h>
#include <inttypes.h>

/** Compute the absolute value of an intmax_t.
 *
 * If the result cannot be represented, the behavior is undefined.
 *
 * @param j Greatest-width integer
 * @return The absolute value of @a j
 */
intmax_t imaxabs(intmax_t j)
{
	intmax_t aj;

	if (j < 0) {
		aj = -j;
		assert(aj >= 0);
	} else {
		aj = j;
	}

	return aj;
}

/** Compute quotient and remainder of greatest-width integer division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom)
{
	imaxdiv_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** @}
 */
