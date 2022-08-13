/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
