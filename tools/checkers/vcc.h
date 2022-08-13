/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
