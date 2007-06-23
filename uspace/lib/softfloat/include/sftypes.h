/*
 * Copyright (c) 2005 Josef Cejka
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

#ifndef __SFTYPES_H__
#define __SFTYPES_H__

#include <endian.h>
#include <stdint.h>

typedef union {
	float f;
	uint32_t binary;

	struct 	{
		#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t sign:1;
		uint32_t exp:8;
		uint32_t fraction:23;
		#elif __BYTE_ORDER == __LITTLE_ENDIAN
		uint32_t fraction:23;
		uint32_t exp:8;
		uint32_t sign:1;
		#else 
			#error "Unknown endians."
		#endif
		} parts __attribute__ ((packed));
 	} float32;
	
typedef union {
	double d;
	uint64_t binary;
	
	struct	{
		#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t sign:1;
		uint64_t exp:11;
		uint64_t fraction:52;
		#elif __BYTE_ORDER == __LITTLE_ENDIAN
		uint64_t fraction:52;
		uint64_t exp:11;
		uint64_t sign:1;
		#else 
			#error "Unknown endians."
		#endif
		} parts __attribute__ ((packed));
	} float64;

#define FLOAT32_MAX 0x7f800000
#define FLOAT32_MIN 0xff800000
#define FLOAT64_MAX
#define FLOAT64_MIN

/* For recognizing NaNs or infinity use isFloat32NaN and is Float32Inf, comparing with this constants is not sufficient */
#define FLOAT32_NAN 0x7FC00001
#define FLOAT32_SIGNAN 0x7F800001
#define FLOAT32_INF 0x7F800000

#define FLOAT64_NAN 0x7FF8000000000001ll
#define FLOAT64_SIGNAN 0x7FF0000000000001ll
#define FLOAT64_INF 0x7FF0000000000000ll

#define FLOAT32_FRACTION_SIZE 23
#define FLOAT64_FRACTION_SIZE 52

#define FLOAT32_HIDDEN_BIT_MASK 0x800000
#define FLOAT64_HIDDEN_BIT_MASK 0x10000000000000ll

#define FLOAT32_MAX_EXPONENT 0xFF
#define FLOAT64_MAX_EXPONENT 0x7FF

#define FLOAT32_BIAS 0x7F
#define FLOAT64_BIAS 0x3FF
#define FLOAT80_BIAS 0x3FFF


#endif


 /** @}
 */

