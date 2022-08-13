/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

// XXX: The definition of `assert()` is not guarded.
// One must not use `#pragma once` in this header.
// This is in accordance with the C standard.

#ifndef _LIBC_ASSERT_H_
#define _LIBC_ASSERT_H_

#include <_bits/decls.h>

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

__C_DECLS_BEGIN;

extern void __helenos_assert_abort(const char *, const char *, unsigned int)
    __attribute__((noreturn));

extern void __helenos_assert_quick_abort(const char *, const char *, unsigned int)
    __attribute__((noreturn));

__C_DECLS_END;

#endif

/** Debugging assert macro
 *
 * If NDEBUG is not set, the assert() macro
 * evaluates expr and if it is false prints
 * error message and terminate program.
 *
 * @param expr Expression which is expected to be true.
 *
 */

#undef assert

#ifndef NDEBUG
#define assert(expr) ((expr) ? (void) 0 : __helenos_assert_abort(#expr, __FILE__, __LINE__))
#else
#define assert(expr) ((void) 0)
#endif

#ifdef _HELENOS_SOURCE

#undef safe_assert

#ifndef NDEBUG
#define safe_assert(expr) ((expr) ? (void) 0 : __helenos_assert_quick_abort(#expr, __FILE__, __LINE__))
#else
#define safe_assert(expr) ((void) 0)
#endif

#endif /* _HELENOS_SOURCE */

/** @}
 */
