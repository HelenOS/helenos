/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#ifndef _MATH_H
#define _MATH_H

#include <limits.h>
#include <stddef.h>
#include <float.h>

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_HELENOS_SOURCE)

#define M_E         2.7182818284590452354
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

/* GNU extensions, but GCC emits calls to them as an optimization. */
void sincos(double, double *, double *);
void sincosf(float, float *, float *);
void sincosl(long double, long double *, long double *);

#endif

#if FLT_EVAL_METHOD == 0
typedef float float_t;
typedef double double_t;
#elif FLT_EVAL_METHOD == 1
typedef double float_t;
typedef double double_t;
#elif FLT_EVAL_METHOD == 2
typedef long double float_t;
typedef long double double_t;
#else
#error Unknown FLT_EVAL_METHOD value.
#endif

#define FP_ILOGB0    INT_MIN
#define FP_ILOGBNAN  INT_MAX

#define FP_NAN        0
#define FP_INFINITE   1
#define FP_NORMAL     2
#define FP_SUBNORMAL  3
#define FP_ZERO       4

int __fpclassify(size_t, ...);
int __signbit(size_t, ...);
int __fcompare(size_t, size_t, ...);

#define __FCOMPARE_EQUAL    1
#define __FCOMPARE_LESS     2
#define __FCOMPARE_GREATER  4

#if defined(__GNUC__) || defined(__clang__)

#define HUGE_VALF  __builtin_huge_valf()
#define HUGE_VAL   __builtin_huge_val()
#define HUGE_VALL  __builtin_huge_vall()
#define INFINITY   __builtin_inff()
#define NAN        __builtin_nan("")

#define fpclassify(x)  __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, (x))
#define isfinite(x)    __builtin_isfinite(x)
#define isinf(x)       __builtin_isinf(x)
#define isnan(x)       __builtin_isnan(x)
#define isnormal(x)    __builtin_isnormal(x)
#define signbit(x)     __builtin_signbit(x)

#define isgreater(x, y)      __builtin_isgreater(x, y)
#define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
#define isless(x, y)         __builtin_isless(x, y)
#define islessequal(x, y)    __builtin_islessequal(x, y)
#define islessgreater(x, y)  __builtin_islessgreater(x, y)
#define isunordered(x, y)    __builtin_isunordered(x, y)

#else  /* defined(__GNUC__) || defined(__clang__) */

#define HUGE_VALF  (1.0f / 0.0f)
#define HUGE_VAL   (1.0 / 0.0)
#define HUGE_VALL  (1.0l / 0.0l)
#define INFINITY   (1.0f / 0.0f)
#define NAN        (0.0 / 0.0)

#define fpclassify(x)  __fpclassify(sizeof(x), (x))
#define isfinite(x)    (fpclassify(x) > FP_INFINITE)
#define isinf(x)       (fpclassify(x) == FP_INFINITE)
#define isnan(x)       (fpclassify(x) == FP_NAN)
#define isnormal(x)    (fpclassify(x) == FP_NORMAL)
#define signbit(x)     __signbit(sizeof(x), (x))

#define isgreater(x, y)      (!!(__fcompare(sizeof(x), sizeof(y), (x), (y)) & __FCOMPARE_GREATER))
#define isgreaterequal(x, y) (!!(__fcompare(sizeof(x), sizeof(y), (x), (y)) & (__FCOMPARE_GREATER | __FCOMPARE_EQUAL))))
#define isless(x, y)         (!!(__fcompare(sizeof(x), sizeof(y), (x), (y)) & __FCOMPARE_LESS))
#define islessequal(x, y)    (!!(__fcompare(sizeof(x), sizeof(y), (x), (y)) & (__FCOMPARE_LESS | __FCOMPARE_EQUAL)))
#define islessgreater(x, y)  (!!(__fcompare(sizeof(x), sizeof(y), (x), (y)) & (__FCOMPARE_LESS | __FCOMPARE_GREATER)))
#define isunordered(x, y)    (!__fcompare(sizeof(x), sizeof(y), (x), (y)))

#endif  /* defined(__GNUC__) || defined(__clang__) */

#define MATH_ERRNO      1
#define MATH_ERREXCEPT  2

#define math_errhandling MATH_ERRNO

double acos(double);
float acosf(float);
long double acosl(long double);
double asin(double);
float asinf(float);
long double asinl(long double);
double atan(double);
float atanf(float);
long double atanl(long double);
double atan2(double, double);
float atan2f(float, float);
long double atan2l(long double, long double);
double cos(double);
float cosf(float);
long double cosl(long double);
double sin(double);
float sinf(float);
long double sinl(long double);
double tan(double);
float tanf(float);
long double tanl(long double);
double acosh(double);
float acoshf(float);
long double acoshl(long double);
double asinh(double);
float asinhf(float);
long double asinhl(long double);
double atanh(double);
float atanhf(float);
long double atanhl(long double);
double cosh(double);
float coshf(float);
long double coshl(long double);
double sinh(double);
float sinhf(float);
long double sinhl(long double);
double tanh(double);
float tanhf(float);
long double tanhl(long double);
double exp(double);
float expf(float);
long double expl(long double);
double exp2(double);
float exp2f(float);
long double exp2l(long double);
double expm1(double);
float expm1f(float);
long double expm1l(long double);
double frexp(double, int *);
float frexpf(float, int *);
long double frexpl(long double, int *);
int ilogb(double);
int ilogbf(float);
int ilogbl(long double);
double ldexp(double, int);
float ldexpf(float, int);
long double ldexpl(long double, int);
double log(double);
float logf(float);
long double logl(long double);
double log10(double);
float log10f(float);
long double log10l(long double);
double log1p(double);
float log1pf(float);
long double log1pl(long double);
double log2(double);
float log2f(float);
long double log2l(long double);
double logb(double);
float logbf(float);
long double logbl(long double);
double modf(double value, double *);
float modff(float value, float *);
long double modfl(long double, long double *);
double scalbn(double, int);
float scalbnf(float, int);
long double scalbnl(long double, int);
double scalbln(double, long int);
float scalblnf(float, long int);
long double scalblnl(long double, long int);
double cbrt(double);
float cbrtf(float);
long double cbrtl(long double);
double fabs(double);
float fabsf(float);
long double fabsl(long double);
double hypot(double, double);
float hypotf(float, float);
long double hypotl(long double, long double);
double pow(double, double);
float powf(float, float);
long double powl(long double, long double);
double sqrt(double);
float sqrtf(float);
long double sqrtl(long double);
double erf(double);
float erff(float);
long double erfl(long double);
double erfc(double);
float erfcf(float);
long double erfcl(long double);
double lgamma(double);
float lgammaf(float);
long double lgammal(long double);
double tgamma(double);
float tgammaf(float);
long double tgammal(long double);
double ceil(double);
float ceilf(float);
long double ceill(long double);
double floor(double);
float floorf(float);
long double floorl(long double);
double nearbyint(double);
float nearbyintf(float);
long double nearbyintl(long double);
double rint(double);
float rintf(float);
long double rintl(long double);
long int lrint(double);
long int lrintf(float);
long int lrintl(long double);
long long int llrint(double);
long long int llrintf(float);
long long int llrintl(long double);
double round(double);
float roundf(float);
long double roundl(long double);
long int lround(double);
long int lroundf(float);
long int lroundl(long double);
long long int llround(double);
long long int llroundf(float);
long long int llroundl(long double);
double trunc(double);
float truncf(float);
long double truncl(long double);
double fmod(double, double);
float fmodf(float, float);
long double fmodl(long double, long double);
double remainder(double, double);
float remainderf(float, float);
long double remainderl(long double, long double);
double remquo(double, double, int *);
float remquof(float, float, int *);
long double remquol(long double, long double, int *);
double copysign(double, double);
float copysignf(float, float);
long double copysignl(long double, long double);
double nan(const char *);
float nanf(const char *);
long double nanl(const char *);
double nextafter(double, double);
float nextafterf(float, float);
long double nextafterl(long double, long double);
double nexttoward(double, long double);
float nexttowardf(float, long double);
long double nexttowardl(long double, long double);
double fdim(double, double);
float fdimf(float, float);
long double fdiml(long double, long double);
double fmax(double, double);
float fmaxf(float, float);
long double fmaxl(long double, long double);
double fmin(double, double);
float fminf(float, float);
long double fminl(long double, long double);
double fma(double, double, double);
float fmaf(float, float, float);
long double fmal(long double, long double, long double);

#if defined(__GNUC__) || defined(__clang__)

#define copysign __builtin_copysign
#define copysignf __builtin_copysignf
#define copysignl __builtin_copysignl

#define nextafter __builtin_nextafter
#define nextafterf __builtin_nextafterf
#define nextafterl __builtin_nextafterl

#endif

#endif /* _MATH_H */
