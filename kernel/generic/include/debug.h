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

#ifndef KERN_DEBUG_H_
#define KERN_DEBUG_H_

#include <log.h>
#include <printf/verify.h>
#include <symtab.h>

#define CALLER  ((uintptr_t) __builtin_return_address(0))

/* An empty printf function to ensure syntactic correctness of disabled debug prints. */
static inline void dummy_printf(const char *fmt, ...) _HELENOS_PRINTF_ATTRIBUTE(1, 2);
static inline void dummy_printf(const char *fmt, ...)
{
}

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
		    "%s() from %s at %s:%u: " format, __func__, \
		    symtab_fmt_name_lookup(CALLER), __FILE__, __LINE__, \
		    ##__VA_ARGS__); \
	} while (0)

#else /* CONFIG_LOG */

#define LOG(format, ...) dummy_printf(format, ##__VA_ARGS__)

#endif /* CONFIG_LOG */

#ifdef CONFIG_TRACE

extern void __cyg_profile_func_enter(void *, void *);
extern void __cyg_profile_func_exit(void *, void *);

#endif /* CONFIG_TRACE */

#endif

/** @}
 */
