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

#endif

/** @}
 */
