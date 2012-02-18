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
/** @file Floating point types and constants.
 */

#ifndef __SFTYPES_H__
#define __SFTYPES_H__

#include <byteorder.h>
#include <stdint.h>

typedef union {
	float f;
	uint32_t binary;
	
	struct {
#if defined(__BE__)
		uint32_t sign : 1;
		uint32_t exp : 8;
		uint32_t fraction : 23;
#elif defined(__LE__)
		uint32_t fraction : 23;
		uint32_t exp : 8;
		uint32_t sign : 1;
#else
	#error Unknown endianess
#endif
	} parts __attribute__ ((packed));
} float32;

typedef union {
	double d;
	uint64_t binary;
	
	struct {
#if defined(__BE__)
		uint64_t sign : 1;
		uint64_t exp : 11;
		uint64_t fraction : 52;
#elif defined(__LE__)
		uint64_t fraction : 52;
		uint64_t exp : 11;
		uint64_t sign : 1;
#else
	#error Unknown endianess
#endif
	} parts __attribute__ ((packed));
} float64;

typedef union {
	long double ld;
	struct {
#if defined(__BE__)
		uint64_t hi;
		uint64_t lo;
#elif defined(__LE__)
		uint64_t lo;
		uint64_t hi;
#else
	#error Unknown endianess
#endif
	} binary;

	struct {
#if defined(__BE__)
		uint64_t sign : 1;
		uint64_t exp : 15;
		uint64_t frac_hi : 48;
		uint64_t frac_lo : 64;
#elif defined(__LE__)
		uint64_t frac_lo : 64;
		uint64_t frac_hi : 48;
		uint64_t exp : 15;
		uint64_t sign : 1;
#else
	#error Unknown endianess
#endif
	} parts __attribute__ ((packed));
} float128;

/*
 * For recognizing NaNs or infinity use specialized comparison functions,
 * comparing with these constants is not sufficient.
 */

#define FLOAT32_NAN     0x7FC00001
#define FLOAT32_SIGNAN  0x7F800001
#define FLOAT32_INF     0x7F800000

#define FLOAT64_NAN     0x7FF8000000000001ll
#define FLOAT64_SIGNAN  0x7FF0000000000001ll
#define FLOAT64_INF     0x7FF0000000000000ll

#define FLOAT128_NAN_HI     0x7FFF800000000000ll
#define FLOAT128_NAN_LO     0x0000000000000001ll
#define FLOAT128_SIGNAN_HI  0x7FFF000000000000ll
#define FLOAT128_SIGNAN_LO  0x0000000000000001ll
#define FLOAT128_INF_HI     0x7FFF000000000000ll
#define FLOAT128_INF_LO     0x0000000000000000ll

#define FLOAT32_FRACTION_SIZE   23
#define FLOAT64_FRACTION_SIZE   52
#define FLOAT128_FRACTION_SIZE 112
#define FLOAT128_FRAC_HI_SIZE   48
#define FLOAT128_FRAC_LO_SIZE   64

#define FLOAT32_HIDDEN_BIT_MASK      0x800000
#define FLOAT64_HIDDEN_BIT_MASK      0x10000000000000ll
#define FLOAT128_HIDDEN_BIT_MASK_HI  0x1000000000000ll
#define FLOAT128_HIDDEN_BIT_MASK_LO  0x0000000000000000ll

#define FLOAT32_MAX_EXPONENT  0xFF
#define FLOAT64_MAX_EXPONENT  0x7FF
#define FLOAT128_MAX_EXPONENT 0x7FFF

#define FLOAT32_BIAS  0x7F
#define FLOAT64_BIAS  0x3FF
#define FLOAT80_BIAS  0x3FFF
#define FLOAT128_BIAS 0x3FFF

#endif

/** @}
 */
