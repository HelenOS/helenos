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
 *
 * The purpose of this file is only to provide prototypes of mathematical
 * functions defined by C standard and by POSIX.
 *
 * It is up to the application to correctly link with either libmath
 * (provided by HelenOS) or by some other math library (such as fdlibm).
 */

#ifndef POSIX_MATH_H_
#define POSIX_MATH_H_

#ifdef __GNUC__
#define HUGE_VAL (__builtin_huge_val())
#endif

extern double ldexp(double, int);
extern double frexp(double, int *);

extern double fabs(double);
extern double floor(double);
extern double ceil(double);
extern double modf(double, double *);
extern double fmod(double, double);
extern double pow(double, double);
extern double exp(double);
extern double frexp(double, int *);
extern double expm1(double);
extern double sqrt(double);
extern double log(double);
extern double log10(double);
extern double sin(double);
extern double sinh(double);
extern double asin(double);
extern double asinh(double);
extern double cos(double);
extern double cosh(double);
extern double acos(double);
extern double acosh(double);
extern double tan(double);
extern double tanh(double);
extern double atan(double);
extern double atanh(double);
extern double atan2(double, double);
extern double copysign(double, double);

#endif /* POSIX_MATH_H_ */

/** @}
 */
