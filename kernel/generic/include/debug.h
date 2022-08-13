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

#ifndef KERN_DEBUG_H_
#define KERN_DEBUG_H_

#include <log.h>
#include <symtab_lookup.h>

#define CALLER  ((uintptr_t) __builtin_return_address(0))

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
