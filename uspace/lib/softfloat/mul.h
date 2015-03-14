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
/** @file Multiplication functions.
 */

#ifndef __MUL_H__
#define __MUL_H__

#include <mathtypes.h>

extern float32 mul_float32(float32, float32);
extern float64 mul_float64(float64, float64);
extern float96 mul_float96(float96, float96);
extern float128 mul_float128(float128, float128);

#ifdef float32_t
extern float32_t __mulsf3(float32_t, float32_t);
extern float32_t __aeabi_fmul(float32_t, float32_t);
#endif

#ifdef float64_t
extern float64_t __muldf3(float64_t, float64_t);
extern float64_t __aeabi_dmul(float64_t, float64_t);
#endif

#ifdef float128_t
extern float128_t __multf3(float128_t, float128_t);
extern void _Qp_mul(float128_t *, float128_t *, float128_t *);
#endif

#endif

/** @}
 */
