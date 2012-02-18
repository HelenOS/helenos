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

extern int isFloat32NaN(float32);
extern int isFloat32SigNaN(float32);

extern int isFloat32Infinity(float32);
extern int isFloat32Zero(float32);

extern int isFloat32eq(float32, float32);
extern int isFloat32lt(float32, float32);
extern int isFloat32gt(float32, float32);

extern int isFloat64NaN(float64);
extern int isFloat64SigNaN(float64);

extern int isFloat64Infinity(float64);
extern int isFloat64Zero(float64);

extern int isFloat64eq(float64, float64);
extern int isFloat64lt(float64, float64);
extern int isFloat64gt(float64, float64);

extern int isFloat128NaN(float128);
extern int isFloat128SigNaN(float128);

extern int isFloat128Infinity(float128);
extern int isFloat128Zero(float128);

extern int isFloat128eq(float128, float128);
extern int isFloat128lt(float128, float128);
extern int isFloat128gt(float128, float128);

#endif

/** @}
 */
