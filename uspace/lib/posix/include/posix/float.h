/*
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

/** @addtogroup libposix
 * @{
 */
/** @file Floating type support.
 */

#ifndef POSIX_FLOAT_H_
#define POSIX_FLOAT_H_

/* Rouding direction -1 - unknown */
#define FLT_ROUNDS (-1)

/* define some standard C constants in terms of GCC built-ins */
#ifdef __GNUC__
	#undef DBL_MANT_DIG
	#define DBL_MANT_DIG __DBL_MANT_DIG__
	#undef DBL_MIN_EXP
	#define DBL_MIN_EXP __DBL_MIN_EXP__
	#undef DBL_MAX_EXP
	#define DBL_MAX_EXP __DBL_MAX_EXP__
	#undef DBL_MAX
	#define DBL_MAX __DBL_MAX__
	#undef DBL_MAX_10_EXP
	#define DBL_MAX_10_EXP __DBL_MAX_10_EXP__
	#undef DBL_MIN_10_EXP
	#define DBL_MIN_10_EXP __DBL_MIN_10_EXP__
	#undef DBL_MIN
	#define DBL_MIN __DBL_MIN__
	#undef DBL_DIG
	#define DBL_DIG __DBL_DIG__
	#undef DBL_EPSILON
	#define DBL_EPSILON __DBL_EPSILON__
	#undef FLT_RADIX
	#define FLT_RADIX __FLT_RADIX__
#endif

#endif /* POSIX_FLOAT_H_ */

/** @}
 */
