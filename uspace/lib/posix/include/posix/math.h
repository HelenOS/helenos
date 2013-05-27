/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file Mathematical operations.
 */

#ifndef POSIX_MATH_H_
#define POSIX_MATH_H_

#ifdef __GNUC__
	#define HUGE_VAL (__builtin_huge_val())
#endif

/* Normalization Functions */
extern double posix_ldexp(double x, int exp);
extern double posix_frexp(double num, int *exp);

double posix_fabs(double x);
double posix_floor(double x);
double posix_modf(double x, double *iptr);
double posix_fmod(double x, double y);
double posix_pow(double x, double y);
double posix_exp(double x);
double posix_sqrt(double x);
double posix_log(double x);
double posix_sin(double x);
double posix_cos(double x);
double posix_atan2(double y, double x);

#ifndef LIBPOSIX_INTERNAL
	#define ldexp posix_ldexp
	#define frexp posix_frexp

	#define fabs posix_fabs
	#define floor posix_floor
	#define modf posix_modf
	#define fmod posix_fmod
	#define pow posix_pow
	#define exp posix_exp
	#define sqrt posix_sqrt
	#define log posix_log
	#define sin posix_sin
	#define cos posix_cos
	#define atan2 posix_atan2
#endif

#endif /* POSIX_MATH_H_ */

/** @}
 */
