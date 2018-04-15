/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

// XXX: The definition of `assert()` is not guarded.
// One must not use `#pragma once` in this header.
// This is in accordance with the C standard.

#ifndef LIBC_ASSERT_H_
#define LIBC_ASSERT_H_

#define static_assert(expr)	_Static_assert(expr, "")

extern void __helenos_assert_abort(const char *, const char *, unsigned int)
    __attribute__((noreturn));

extern void __helenos_assert_quick_abort(const char *, const char *, unsigned int)
    __attribute__((noreturn));

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
