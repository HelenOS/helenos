/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup libmath
 * @{
 */
/** @file
 */

#include <math.h>
#include <trig.h>

#define TAYLOR_DEGREE  13

/** Precomputed values for factorial (starting from 1!) */
static double factorials[TAYLOR_DEGREE] = {
	1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800,
	479001600, 6227020800
};

/** Sine approximation by Taylor series
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
static double taylor_sin(double arg)
{
	double ret = 0;
	double nom = 1;
	
	for (unsigned int i = 0; i < TAYLOR_DEGREE; i++) {
		nom *= arg;
		
		if ((i % 4) == 0)
			ret += nom / factorials[i];
		else if ((i % 4) == 2)
			ret -= nom / factorials[i];
	}
	
	return ret;
}

/** Cosine approximation by Taylor series
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
static double taylor_cos(double arg)
{
	double ret = 1;
	double nom = 1;
	
	for (unsigned int i = 0; i < TAYLOR_DEGREE; i++) {
		nom *= arg;
		
		if ((i % 4) == 1)
			ret -= nom / factorials[i];
		else if ((i % 4) == 3)
			ret += nom / factorials[i];
	}
	
	return ret;
}

/** Sine value for values within base period
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
static double base_sin(double arg)
{
	unsigned int period = arg / (M_PI / 4);
	
	switch (period) {
	case 0:
		return taylor_sin(arg);
	case 1:
	case 2:
		return taylor_cos(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_sin(arg - M_PI);
	case 5:
	case 6:
		return -taylor_cos(arg - 3 * M_PI / 2);
	default:
		return taylor_sin(arg - 2 * M_PI);
	}
}

/** Cosine value for values within base period
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
static double base_cos(double arg)
{
	unsigned int period = arg / (M_PI / 4);
	
	switch (period) {
	case 0:
		return taylor_cos(arg);
	case 1:
	case 2:
		return taylor_sin(arg - M_PI / 2);
	case 3:
	case 4:
		return -taylor_cos(arg - M_PI);
	case 5:
	case 6:
		return -taylor_sin(arg - 3 * M_PI / 2);
	default:
		return taylor_cos(arg - 2 * M_PI);
	}
}

/** Double precision sine
 *
 * Compute sine value.
 *
 * @param arg Sine argument.
 *
 * @return Sine value.
 *
 */
double double_sin(double arg)
{
	double base_arg = fmod(arg, 2 * M_PI);
	
	if (base_arg < 0)
		return -base_sin(-base_arg);
	
	return base_sin(base_arg);
}

/** Double precision cosine
 *
 * Compute cosine value.
 *
 * @param arg Cosine argument.
 *
 * @return Cosine value.
 *
 */
double double_cos(double arg)
{
	double base_arg = fmod(arg, 2 * M_PI);
	
	if (base_arg < 0)
		return base_cos(-base_arg);
	
	return base_cos(base_arg);
}

/** @}
 */
