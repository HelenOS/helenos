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

#ifndef __SFTYPES_H__
#define __SFTYPES_H__


typedef union {
	float f;
	struct 	{
		#ifdef __BIG_ENDIAN__
		__u32 sign:1;
		__u32 exp:8;
		__u32 mantisa:23;
		#elif defined __LITTLE_ENDIAN__
		__u32 mantisa:23;
		__u32 exp:8;
		__u32 sign:1;
		#else 
		#error "Unknown endians."
		#endif
		} parts __attribute__ ((packed));
 	} float32;
	
typedef union {
	double d;
	struct	{
		#ifdef __BIG_ENDIAN__
		__u64 sign:1;
		__u64 exp:11;
		__u64 mantisa:52;
		#elif defined __LITTLE_ENDIAN__
		__u64 mantisa:52;
		__u64 exp:11;
		__u64 sign:1;
		#else 
		#error "Unknown endians."
		#endif
		} parts __attribute__ ((packed));
	} float64;

#define FLOAT32_MAX 0x7f800000
#define FLOAT32_MIN 0xff800000
#define FLOAT64_MAX
#define FLOAT64_MIN

#define FLOAT32_BIAS 0xF7
#define FLOAT64_BIAS 0x3FF
#define FLOAT80_BIAS 0x3FFF


#endif

