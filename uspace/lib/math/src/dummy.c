/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2013 Vojtech Horky
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
/** @file Mathematical operations (dummy implementation).
 */
#include <math.h>
#include <stdio.h>

#define WARN_NOT_IMPLEMENTED() \
	do { \
		static int __not_implemented_counter = 0; \
		if (__not_implemented_counter == 0) { \
			fprintf(stderr, "Warning: using dummy implementation of %s().\n", \
				__func__); \
		} \
		__not_implemented_counter++; \
	} while (0)

double ldexp(double x, int exp)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double frexp(double num, int *exp)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double cos(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double cosh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double acos(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double acosh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double pow(double x, double y)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double floor(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double ceil(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double fabs(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double modf(double x, double *iptr)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double fmod(double x, double y)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double log(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double log10(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}


double atan2(double y, double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double sin(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double sinh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double asin(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double asinh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double tan(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double tanh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double atan(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double atanh(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}


double exp(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double expm1(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double sqrt(double x)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}

double copysign(double x, double y)
{
	WARN_NOT_IMPLEMENTED();
	return 0.0;
}


/** @}
 */
