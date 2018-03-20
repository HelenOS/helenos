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
/** @file Conversion of precision and conversion between integers and floats.
 */

#ifndef __CONVERSION_H__
#define __CONVERSION_H__

#include <mathtypes.h>

extern float64 float32_to_float64(float32);
extern float96 float32_to_float96(float32);
extern float128 float32_to_float128(float32);
extern float96 float64_to_float96(float64);
extern float128 float64_to_float128(float64);
extern float128 float96_to_float128(float96);

extern float32 float64_to_float32(float64);
extern float32 float96_to_float32(float96);
extern float64 float96_to_float64(float96);
extern float32 float128_to_float32(float128);
extern float64 float128_to_float64(float128);
extern float96 float128_to_float96(float128);

extern uint32_t float32_to_uint32(float32);
extern int32_t float32_to_int32(float32);

extern uint64_t float32_to_uint64(float32);
extern int64_t float32_to_int64(float32);

extern uint32_t float64_to_uint32(float64);
extern int32_t float64_to_int32(float64);

extern uint64_t float64_to_uint64(float64);
extern int64_t float64_to_int64(float64);

extern uint32_t float96_to_uint32(float96);
extern int32_t float96_to_int32(float96);

extern uint64_t float96_to_uint64(float96);
extern int64_t float96_to_int64(float96);

extern uint32_t float128_to_uint32(float128);
extern int32_t float128_to_int32(float128);

extern uint64_t float128_to_uint64(float128);
extern int64_t float128_to_int64(float128);

extern float32 uint32_to_float32(uint32_t);
extern float32 int32_to_float32(int32_t);

extern float32 uint64_to_float32(uint64_t);
extern float32 int64_to_float32(int64_t);

extern float64 uint32_to_float64(uint32_t);
extern float64 int32_to_float64(int32_t);

extern float64 uint64_to_float64(uint64_t);
extern float64 int64_to_float64(int64_t);

extern float96 uint32_to_float96(uint32_t);
extern float96 int32_to_float96(int32_t);

extern float96 uint64_to_float96(uint64_t);
extern float96 int64_to_float96(int64_t);

extern float128 uint32_to_float128(uint32_t);
extern float128 int32_to_float128(int32_t);

extern float128 uint64_to_float128(uint64_t);
extern float128 int64_to_float128(int64_t);

#ifdef float32_t
extern float32_t __floatsisf(int32_t);
extern float32_t __floatdisf(int64_t);
extern float32_t __floatunsisf(uint32_t);
extern float32_t __floatundisf(uint64_t);
extern int32_t __fixsfsi(float32_t);
extern int64_t __fixsfdi(float32_t);
extern uint32_t __fixunssfsi(float32_t);
extern uint64_t __fixunssfdi(float32_t);
extern int32_t __aeabi_f2iz(float32_t);
extern int64_t __aeabi_f2lz(float32_t);
extern uint32_t __aeabi_f2uiz(float32_t);
extern float32_t __aeabi_i2f(int32_t);
extern float32_t __aeabi_l2f(int64_t);
extern float32_t __aeabi_ui2f(uint32_t);
extern float32_t __aeabi_ul2f(uint64_t);
#endif

#ifdef float64_t
extern float64_t __floatsidf(int32_t);
extern float64_t __floatdidf(int64_t);
extern float64_t __floatunsidf(uint32_t);
extern float64_t __floatundidf(uint64_t);
extern int32_t __fixdfsi(float64_t);
extern int64_t __fixdfdi(float64_t);
extern uint32_t __fixunsdfsi(float64_t);
extern uint64_t __fixunsdfdi(float64_t);
extern float64_t __aeabi_i2d(int32_t);
extern float64_t __aeabi_ui2d(uint32_t);
extern float64_t __aeabi_l2d(int64_t);
extern int32_t __aeabi_d2iz(float64_t);
extern int64_t __aeabi_d2lz(float64_t);
extern uint32_t __aeabi_d2uiz(float64_t);
#endif

#ifdef float128_t
extern float128_t __floatsitf(int32_t);
extern float128_t __floatditf(int64_t);
extern float128_t __floatunsitf(uint32_t);
extern float128_t __floatunditf(uint64_t);
extern int32_t __fixtfsi(float128_t);
extern int64_t __fixtfdi(float128_t);
extern uint32_t __fixunstfsi(float128_t);
extern uint64_t __fixunstfdi(float128_t);
extern int32_t _Qp_qtoi(float128_t *);
extern int64_t _Qp_qtox(float128_t *);
extern uint32_t _Qp_qtoui(float128_t *);
extern uint64_t _Qp_qtoux(float128_t *);
extern void _Qp_itoq(float128_t *, int32_t);
extern void _Qp_xtoq(float128_t *, int64_t);
extern void _Qp_uitoq(float128_t *, uint32_t);
extern void _Qp_uxtoq(float128_t *, uint64_t);
#endif

#if (defined(float32_t) && defined(float64_t))
extern float32_t __truncdfsf2(float64_t);
extern float64_t __extendsfdf2(float32_t);
extern float64_t __aeabi_f2d(float32_t);
extern float32_t __aeabi_d2f(float64_t);
#endif

#if (defined(float32_t) && defined(float128_t))
extern float32_t __trunctfsf2(float128_t);
extern float128_t __extendsftf2(float32_t);
extern void _Qp_stoq(float128_t *, float32_t);
extern float32_t _Qp_qtos(float128_t *);
#endif

#if (defined(float64_t) && defined(float128_t))
extern float64_t __trunctfdf2(float128_t);
extern float128_t __extenddftf2(float64_t);
extern void _Qp_dtoq(float128_t *, float64_t);
extern float64_t _Qp_qtod(float128_t *);
#endif

#endif

/** @}
 */
