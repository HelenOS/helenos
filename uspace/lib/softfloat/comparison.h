/*
 * Copyright (c) 2005 Josef Cejka
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

/** @addtogroup softfloat
 * @{
 */
/** @file Comparison functions.
 */

#ifndef __COMPARISON_H__
#define __COMPARISON_H__

#include <mathtypes.h>

extern int is_float32_nan(float32);
extern int is_float32_signan(float32);

extern int is_float32_infinity(float32);
extern int is_float32_zero(float32);

extern int is_float32_eq(float32, float32);
extern int is_float32_lt(float32, float32);
extern int is_float32_gt(float32, float32);

extern int is_float64_nan(float64);
extern int is_float64_signan(float64);

extern int is_float64_infinity(float64);
extern int is_float64_zero(float64);

extern int is_float64_eq(float64, float64);
extern int is_float64_lt(float64, float64);
extern int is_float64_gt(float64, float64);

extern int is_float96_nan(float96);
extern int is_float96_signan(float96);

extern int is_float96_infinity(float96);
extern int is_float96_zero(float96);

extern int is_float96_eq(float96, float96);
extern int is_float96_lt(float96, float96);
extern int is_float96_gt(float96, float96);

extern int is_float128_nan(float128);
extern int is_float128_signan(float128);

extern int is_float128_infinity(float128);
extern int is_float128_zero(float128);

extern int is_float128_eq(float128, float128);
extern int is_float128_lt(float128, float128);
extern int is_float128_gt(float128, float128);

#ifdef float32_t
extern int __gtsf2(float32_t, float32_t);
extern int __gesf2(float32_t, float32_t);
extern int __ltsf2(float32_t, float32_t);
extern int __lesf2(float32_t, float32_t);
extern int __eqsf2(float32_t, float32_t);
extern int __nesf2(float32_t, float32_t);
extern int __cmpsf2(float32_t, float32_t);
extern int __unordsf2(float32_t, float32_t);
extern int __aeabi_fcmpgt(float32_t, float32_t);
extern int __aeabi_fcmplt(float32_t, float32_t);
extern int __aeabi_fcmpge(float32_t, float32_t);
extern int __aeabi_fcmple(float32_t, float32_t);
extern int __aeabi_fcmpeq(float32_t, float32_t);
#endif

#ifdef float64_t
extern int __gtdf2(float64_t, float64_t);
extern int __gedf2(float64_t, float64_t);
extern int __ltdf2(float64_t, float64_t);
extern int __ledf2(float64_t, float64_t);
extern int __eqdf2(float64_t, float64_t);
extern int __nedf2(float64_t, float64_t);
extern int __cmpdf2(float64_t, float64_t);
extern int __unorddf2(float64_t, float64_t);
extern int __aeabi_dcmplt(float64_t, float64_t);
extern int __aeabi_dcmpeq(float64_t, float64_t);
extern int __aeabi_dcmpgt(float64_t, float64_t);
extern int __aeabi_dcmpge(float64_t, float64_t);
extern int __aeabi_dcmple(float64_t, float64_t);
#endif

#ifdef float128_t
extern int __gttf2(float128_t, float128_t);
extern int __getf2(float128_t, float128_t);
extern int __lttf2(float128_t, float128_t);
extern int __letf2(float128_t, float128_t);
extern int __eqtf2(float128_t, float128_t);
extern int __netf2(float128_t, float128_t);
extern int __cmptf2(float128_t, float128_t);
extern int __unordtf2(float128_t, float128_t);
extern int _Qp_cmp(float128_t *, float128_t *);
extern int _Qp_cmpe(float128_t *, float128_t *);
extern int _Qp_fgt(float128_t *, float128_t *);
extern int _Qp_fge(float128_t *, float128_t *);
extern int _Qp_flt(float128_t *, float128_t *);
extern int _Qp_fle(float128_t *, float128_t *);
extern int _Qp_feq(float128_t *, float128_t *);
extern int _Qp_fne(float128_t *, float128_t *);

#endif

#endif

/** @}
 */
