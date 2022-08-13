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

#define TAYLOR_DEGREE_32 13
#define TAYLOR_DEGREE_64 21

/** Precomputed values for factorial (starting from 1!) */
static double factorials[TAYLOR_DEGREE_64] = {
	1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800,
	479001600, 6227020800.0L, 87178291200.0L, 1307674368000.0L,
	20922789888000.0L, 355687428096000.0L, 6402373705728000.0L,
	121645100408832000.0L, 2432902008176640000.0L, 51090942171709440000.0L
};

/** Sine approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of sine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Sine argument.
 *
 * @return Sine value approximation.
 *
 */
static float taylor_sin_32(float arg)
{
	float ret = 0;
	float nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_32; i++) {
		nom *= arg;

		if ((i % 4) == 0)
			ret += nom / factorials[i];
		else if ((i % 4) == 2)
			ret -= nom / factorials[i];
	}

	return ret;
}

/** Sine approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of sine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Sine argument.
 *
 * @return Sine value approximation.
 *
 */
static double taylor_sin_64(double arg)
{
	double ret = 0;
	double nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_64; i++) {
		nom *= arg;

		if ((i % 4) == 0)
			ret += nom / factorials[i];
		else if ((i % 4) == 2)
			ret -= nom / factorials[i];
	}

	return ret;
}

/** Cosine approximation by Taylor series (32-bit floating point)
 *
 * Compute the approximation of cosine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value approximation.
 *
 */
static float taylor_cos_32(float arg)
{
	float ret = 1;
	float nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_32; i++) {
		nom *= arg;

		if ((i % 4) == 1)
			ret -= nom / factorials[i];
		else if ((i % 4) == 3)
			ret += nom / factorials[i];
	}

	return ret;
}

/** Cosine approximation by Taylor series (64-bit floating point)
 *
 * Compute the approximation of cosine by a Taylor
 * series (using the first TAYLOR_DEGREE terms).
 * The approximation is reasonably accurate for
 * arguments within the interval [-pi/4, pi/4].
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value approximation.
 *
 */
static double taylor_cos_64(double arg)
{
	double ret = 1;
	double nom = 1;

	for (unsigned int i = 0; i < TAYLOR_DEGREE_64; i++) {
		nom *= arg;

		if ((i % 4) == 1)
			ret -= nom / factorials[i];
		else if ((i % 4) == 3)
			ret += nom / factorials[i];
	}

	return ret;
}

/** Sine value for values within base period (32-bit floating point)
 *
 * Compute the value of sine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
float __math_base_sin_32(float arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_sin_32(arg);
	case 1:
	case 2:
		return taylor_cos_32(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_sin_32(arg - M_PI);
	case 5:
	case 6:
		return -taylor_cos_32(arg - 3 * M_PI / 2);
	default:
		return taylor_sin_32(arg - 2 * M_PI);
	}
}

/** Sine value for values within base period (64-bit floating point)
 *
 * Compute the value of sine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
double __math_base_sin_64(double arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_sin_64(arg);
	case 1:
	case 2:
		return taylor_cos_64(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_sin_64(arg - M_PI);
	case 5:
	case 6:
		return -taylor_cos_64(arg - 3 * M_PI / 2);
	default:
		return taylor_sin_64(arg - 2 * M_PI);
	}
}

/** Cosine value for values within base period (32-bit floating point)
 *
 * Compute the value of cosine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
float __math_base_cos_32(float arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_cos_32(arg);
	case 1:
	case 2:
		return -taylor_sin_32(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_cos_32(arg - M_PI);
	case 5:
	case 6:
		return taylor_sin_32(arg - 3 * M_PI / 2);
	default:
		return taylor_cos_32(arg - 2 * M_PI);
	}
}

/** Cosine value for values within base period (64-bit floating point)
 *
 * Compute the value of cosine for arguments within
 * the base period [0, 2pi]. For arguments outside
 * the base period the returned values can be
 * very inaccurate or even completely wrong.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
double __math_base_cos_64(double arg)
{
	unsigned int period = arg / (M_PI / 4);

	switch (period) {
	case 0:
		return taylor_cos_64(arg);
	case 1:
	case 2:
		return -taylor_sin_64(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_cos_64(arg - M_PI);
	case 5:
	case 6:
		return taylor_sin_64(arg - 3 * M_PI / 2);
	default:
		return taylor_cos_64(arg - 2 * M_PI);
	}
}

/** @}
 */
