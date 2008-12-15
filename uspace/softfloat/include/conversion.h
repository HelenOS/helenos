/*
 * Copyright (C) 2005 Josef Cejka
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
/** @file
 */

#ifndef __CONVERSION_H__
#define __CONVERSION_H__

float64 convertFloat32ToFloat64(float32 a);

float32 convertFloat64ToFloat32(float64 a);

uint32_t float32_to_uint32(float32 a);
int32_t float32_to_int32(float32 a);

uint64_t float32_to_uint64(float32 a);
int64_t float32_to_int64(float32 a);

uint64_t float64_to_uint64(float64 a);
int64_t float64_to_int64(float64 a);

uint32_t float64_to_uint32(float64 a);
int32_t float64_to_int32(float64 a);

float32 uint32_to_float32(uint32_t i);
float32 int32_to_float32(int32_t i);

float32 uint64_to_float32(uint64_t i);
float32 int64_to_float32(int64_t i);

float64 uint32_to_float64(uint32_t i);
float64 int32_to_float64(int32_t i);

float64 uint64_to_float64(uint64_t i);
float64 int64_to_float64(int64_t i);

#endif


 /** @}
 */

