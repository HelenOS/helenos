/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup softfloatia32 ia32	
 * @ingroup sfl
 * @brief softfloat architecture dependent definitions 
 * @{
 */
/** @file
 */

#ifndef __SOFTFLOAT_FUNCTIONS_H__
#define __SOFTFLOAT_FUNCTIONS_H__

#define float32_to_int(X) float32_to_int32(X);
#define float32_to_long(X) float32_to_int32(X);
#define float32_to_longlong(X) float32_to_int64(X);

#define float64_to_int(X) float64_to_int32(X);
#define float64_to_long(X) float64_to_int32(X);
#define float64_to_longlong(X) float64_to_int64(X);

#define float128_to_int(X) float128_to_int32(X);
#define float128_to_long(X) float128_to_int32(X);
#define float128_to_longlong(X) float128_to_int64(X);

#define float32_to_uint(X) float32_to_uint32(X);
#define float32_to_ulong(X) float32_to_uint32(X);
#define float32_to_ulonglong(X) float32_to_uint64(X);

#define float64_to_uint(X) float64_to_uint32(X);
#define float64_to_ulong(X) float64_to_uint32(X);
#define float64_to_ulonglong(X) float64_to_uint64(X);

#define float128_to_uint(X) float128_to_uint32(X);
#define float128_to_ulong(X) float128_to_uint32(X);
#define float128_to_ulonglong(X) float128_to_uint64(X);

#define int_to_float32(X) int32_to_float32(X);
#define long_to_float32(X) int32_to_float32(X);
#define longlong_to_float32(X) int64_to_float32(X);

#define int_to_float64(X) int32_to_float64(X);
#define long_to_float64(X) int32_to_float64(X);
#define longlong_to_float64(X) int64_to_float64(X);

#define int_to_float128(X) int32_to_float128(X);
#define long_to_float128(X) int32_to_float128(X);
#define longlong_to_float128(X) int64_to_float128(X);

#define uint_to_float32(X) uint32_to_float32(X);
#define ulong_to_float32(X) uint32_to_float32(X);
#define ulonglong_to_float32(X) uint64_to_float32(X);

#define uint_to_float64(X) uint32_to_float64(X);
#define ulong_to_float64(X) uint32_to_float64(X);
#define ulonglong_to_float64(X) uint64_to_float64(X);

#define uint_to_float128(X) uint32_to_float128(X);
#define ulong_to_float128(X) uint32_to_float128(X);
#define ulonglong_to_float128(X) uint64_to_float128(X);

#endif

/** @}
 */
