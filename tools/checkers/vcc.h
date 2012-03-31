/*
 * Copyright (c) 2010 Martin Decky
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

/** @file
 *
 * VCC specifications tight to the HelenOS-specific annotations.
 *
 */

#ifndef HELENOS_VCC_H_
#define HELENOS_VCC_H_

typedef _Bool bool;

#define __concat_identifiers_str(a, b)  a ## b
#define __concat_identifiers(a, b)      __concat_identifiers_str(a, b)

#define __specification_attr(key, value) \
	__declspec(System.Diagnostics.Contracts.CodeContract.StringVccAttr, \
	    key, value)

#define __specification_type(name) \
	__specification( \
		typedef struct __concat_identifiers(_vcc_math_type_, name) { \
			char _vcc_marker_for_math_type; \
		} __concat_identifiers(\, name); \
	)

__specification(typedef void * \object;)
__specification(typedef __int64 \integer;)
__specification(typedef unsigned __int64 \size_t;)

__specification_type(objset)

__specification(struct \TypeState {
	__specification(ghost \integer \claim_count;)
	__specification(ghost bool \consistent;)
	__specification(ghost \objset \owns;)
	__specification(ghost \object \owner;)
	__specification(ghost bool \valid;)
};)

__specification(bool \extent_mutable(\object);)
__specification(\objset \extent(\object);)
__specification(\objset \array_range(\object, \size_t);)
__specification(bool \mutable_array(\object, \size_t);)

#endif

/** @}
 */
