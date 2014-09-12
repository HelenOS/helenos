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

/** @addtogroup genericdebug
 * @{
 */
/** @file
 */

#ifndef KERN_DEBUG_H_
#define KERN_DEBUG_H_

#include <panic.h>
#include <log.h>
#include <symtab_lookup.h>

#define CALLER  ((uintptr_t) __builtin_return_address(0))

#ifdef CONFIG_DEBUG

/** Debugging ASSERT macro
 *
 * If CONFIG_DEBUG is set, the ASSERT() macro
 * evaluates expr and if it is false raises
 * kernel panic.
 *
 * @param expr Expression which is expected to be true.
 *
 */
#define ASSERT(expr) \
	do { \
		if (!(expr)) \
			panic_assert("%s() at %s:%u:\n%s", \
			    __func__, __FILE__, __LINE__, #expr); \
	} while (0)

/** Debugging verbose ASSERT macro
 *
 * If CONFIG_DEBUG is set, the ASSERT() macro
 * evaluates expr and if it is false raises
 * kernel panic. The panic message contains also
 * the supplied message.
 *
 * @param expr Expression which is expected to be true.
 * @param msg  Additional message to show (string).
 *
 */
#define ASSERT_VERBOSE(expr, msg) \
	do { \
		if (!(expr)) \
			panic_assert("%s() at %s:%u:\n%s, %s", \
			    __func__, __FILE__, __LINE__, #expr, msg); \
	} while (0)

/** Static assert macro
 *
 */
#define STATIC_ASSERT(expr) \
	_Static_assert(expr, "")

#define STATIC_ASSERT_VERBOSE(expr, msg) \
	_Static_assert(expr, msg)


#else /* CONFIG_DEBUG */

#define ASSERT(expr)
#define ASSERT_VERBOSE(expr, msg)
#define STATIC_ASSERT(expr)
#define STATIC_ASSERT_VERBOSE(expr, msg)

#endif /* CONFIG_DEBUG */

#ifdef CONFIG_LOG

/** Extensive logging output macro
 *
 * If CONFIG_LOG is set, the LOG() macro
 * will print whatever message is indicated plus
 * an information about the location.
 *
 */
#define LOG(format, ...) \
	do { \
		log(LF_OTHER, LVL_DEBUG, \
		    "%s() from %s at %s:%u: " format,__func__, \
		    symtab_fmt_name_lookup(CALLER), __FILE__, __LINE__, \
		    ##__VA_ARGS__); \
	} while (0)

#else /* CONFIG_LOG */

#define LOG(format, ...)

#endif /* CONFIG_LOG */

#ifdef CONFIG_TRACE

extern void __cyg_profile_func_enter(void *, void *);
extern void __cyg_profile_func_exit(void *, void *);

#endif /* CONFIG_TRACE */

#endif

/** @}
 */
