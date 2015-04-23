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

/** @addtogroup libmath
 * @{
 */
/** @file Floating point types and constants.
 */

#ifndef LIBMATH_MATHTYPES_H_
#define LIBMATH_MATHTYPES_H_

#include <byteorder.h>
#include <stdint.h>

/*
 * For recognizing NaNs or infinity use specialized comparison
 * functions, comparing with these constants is not sufficient.
 */

#define FLOAT32_NAN     UINT32_C(0x7fc00001)
#define FLOAT32_SIGNAN  UINT32_C(0x7f800001)
#define FLOAT32_INF     UINT32_C(0x7f800000)

#define FLOAT64_NAN     UINT64_C(0x7ff8000000000001)
#define FLOAT64_SIGNAN  UINT64_C(0x7ff0000000000001)
#define FLOAT64_INF     UINT64_C(0x7ff0000000000000)

#define FLOAT96_NAN_HI     UINT64_C(0x7fff80000000)
#define FLOAT96_NAN_LO     UINT32_C(0x00010000)
#define FLOAT96_SIGNAN_HI  UINT64_C(0x7fff00000000)
#define FLOAT96_SIGNAN_LO  UINT32_C(0x00010000)

#define FLOAT128_NAN_HI     UINT64_C(0x7fff800000000000)
#define FLOAT128_NAN_LO     UINT64_C(0x0000000000000001)
#define FLOAT128_SIGNAN_HI  UINT64_C(0x7fff000000000000)
#define FLOAT128_SIGNAN_LO  UINT64_C(0x0000000000000001)
#define FLOAT128_INF_HI     UINT64_C(0x7fff000000000000)
#define FLOAT128_INF_LO     UINT64_C(0x0000000000000000)

#define FLOAT32_FRACTION_SIZE   23
#define FLOAT64_FRACTION_SIZE   52
#define FLOAT96_FRACTION_SIZE   64
#define FLOAT128_FRACTION_SIZE  112
#define FLOAT128_FRAC_HI_SIZE   48
#define FLOAT128_FRAC_LO_SIZE   64

#define FLOAT32_HIDDEN_BIT_MASK      UINT32_C(0x800000)
#define FLOAT64_HIDDEN_BIT_MASK      UINT64_C(0x10000000000000)
#define FLOAT128_HIDDEN_BIT_MASK_HI  UINT64_C(0x1000000000000)
#define FLOAT128_HIDDEN_BIT_MASK_LO  UINT64_C(0x0000000000000000)

#define FLOAT32_MAX_EXPONENT   0xff
#define FLOAT64_MAX_EXPONENT   0x7ff
#define FLOAT96_MAX_EXPONENT   0x7fff
#define FLOAT128_MAX_EXPONENT  0x7fff

#define FLOAT32_BIAS   0x7f
#define FLOAT64_BIAS   0x3ff
#define FLOAT96_BIAS   0x3fff
#define FLOAT128_BIAS  0x3fff

#if defined(__BE__)

typedef union {
	uint32_t bin;
	
	struct {
		uint32_t sign : 1;
		uint32_t exp : 8;
		uint32_t fraction : 23;
	} parts __attribute__((packed));
} float32;

typedef union {
	uint64_t bin;
	
	struct {
		uint64_t sign : 1;
		uint64_t exp : 11;
		uint64_t fraction : 52;
	} parts __attribute__((packed));
} float64;

typedef union {
	struct {
		uint64_t hi;
		uint32_t lo;
	} bin __attribute__((packed));
	
	struct {
		uint64_t padding : 16;
		uint64_t sign : 1;
		uint64_t exp : 15;
		uint64_t fraction : 64;
	} parts __attribute__((packed));
} float96;

typedef union {
	struct {
		uint64_t hi;
		uint64_t lo;
	} bin __attribute__((packed));
	
	struct {
		uint64_t sign : 1;
		uint64_t exp : 15;
		uint64_t frac_hi : 48;
		uint64_t frac_lo : 64;
	} parts __attribute__((packed));
} float128;

#elif defined(__LE__)

typedef union {
	uint32_t bin;
	
	struct {
		uint32_t fraction : 23;
		uint32_t exp : 8;
		uint32_t sign : 1;
	} parts __attribute__((packed));
} float32;

typedef union {
	uint64_t bin;
	
	struct {
		uint64_t fraction : 52;
		uint64_t exp : 11;
		uint64_t sign : 1;
	} parts __attribute__((packed));
} float64;

typedef union {
	struct {
		uint32_t lo;
		uint64_t hi;
	} bin __attribute__((packed));
	
	struct {
		uint64_t fraction : 64;
		uint64_t exp : 15;
		uint64_t sign : 1;
		uint64_t padding : 16;
	} parts __attribute__((packed));
} float96;

typedef union {
	struct {
		uint64_t lo;
		uint64_t hi;
	} bin __attribute__((packed));
	
	struct {
		uint64_t frac_lo : 64;
		uint64_t frac_hi : 48;
		uint64_t exp : 15;
		uint64_t sign : 1;
	} parts __attribute__((packed));
} float128;

#else
	#error Unknown endianess
#endif


#if defined(FLOAT_SIZE_32)

#ifndef float32_t
	#define float32_t  float
#endif

#elif defined(FLOAT_SIZE_64)

#ifndef float64_t
	#define float64_t  float
#endif

#elif defined(FLOAT_SIZE_96)

#ifndef float96_t
	#define float96_t  float
#endif

#elif defined(FLOAT_SIZE_128)

#ifndef float128_t
	#define float128_t  float
#endif

#endif


#if defined(DOUBLE_SIZE_32)

#ifndef float32_t
	#define float32_t  double
#endif

#elif defined(DOUBLE_SIZE_64)

#ifndef float64_t
	#define float64_t  double
#endif

#elif defined(DOUBLE_SIZE_96)

#ifndef float96_t
	#define float96_t  double
#endif

#elif defined(DOUBLE_SIZE_128)

#ifndef float128_t
	#define float128_t  double
#endif

#endif


#if defined(LONG_DOUBLE_SIZE_32)

#ifndef float32_t
	#define float32_t  long double
#endif

#elif defined(LONG_DOUBLE_SIZE_64)

#ifndef float64_t
	#define float64_t  long double
#endif

#elif defined(LONG_DOUBLE_SIZE_96)

#ifndef float96_t
	#define float96_t  long double
#endif

#elif defined(LONG_DOUBLE_SIZE_128)

#ifndef float128_t
	#define float128_t  long double
#endif

#endif


#ifdef float32_t

typedef union {
	float32_t val;
	float32 data;
} float32_u;

#endif

#ifdef float64_t

typedef union {
	float64_t val;
	float64 data;
} float64_u;

#endif

#ifdef float96_t

typedef union {
	float96_t val;
	float96 data;
} float96_u;

#endif

#ifdef float128_t

typedef union {
	float128_t val;
	float128 data;
} float128_u;

#endif

#endif

/** @}
 */
