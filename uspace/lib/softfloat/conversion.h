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

#endif

/** @}
 */
