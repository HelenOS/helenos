/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup kernel_generic_debug
 * @{
 */
/** @file
 */

#ifndef KERN_ASSERT_H_
#define KERN_ASSERT_H_

#include <panic.h>

#ifdef CONFIG_DEBUG

/** Debugging assert macro
 *
 * If CONFIG_DEBUG is set, the assert() macro
 * evaluates expr and if it is false raises
 * kernel panic.
 *
 * @param expr Expression which is expected to be true.
 *
 */
#define assert(expr) \
	do { \
		if (!(expr)) \
			panic_assert("%s() at %s:%u:\n%s", \
			    __func__, __FILE__, __LINE__, #expr); \
	} while (0)

/** Debugging verbose assert macro
 *
 * If CONFIG_DEBUG is set, the assert_verbose() macro
 * evaluates expr and if it is false raises
 * kernel panic. The panic message contains also
 * the supplied message.
 *
 * @param expr Expression which is expected to be true.
 * @param msg  Additional message to show (string).
 *
 */
#define assert_verbose(expr, msg) \
	do { \
		if (!(expr)) \
			panic_assert("%s() at %s:%u:\n%s, %s", \
			    __func__, __FILE__, __LINE__, #expr, msg); \
	} while (0)

/** Static assert macro
 *
 */
#define static_assert \
	_Static_assert

#else /* CONFIG_DEBUG */

#define assert(expr)
#define assert_verbose(expr, msg)
#define static_assert(expr, msg)

#endif /* CONFIG_DEBUG */

#endif

/** @}
 */
