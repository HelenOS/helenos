/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_PANIC_H_
#define KERN_PANIC_H_

#include <typedefs.h>
#include <stacktrace.h>
#include <print.h>

#ifdef CONFIG_DEBUG

#define panic(format, ...) \
	do { \
		silent = false; \
		printf("Kernel panic in %s() at %s:%u\n", \
		    __func__, __FILE__, __LINE__); \
		stack_trace(); \
		panic_printf("Panic message: " format "\n", \
		    ##__VA_ARGS__);\
	} while (0)

#else /* CONFIG_DEBUG */

#define panic(format, ...) \
	do { \
		silent = false; \
		panic_printf("Kernel panic: " format "\n", ##__VA_ARGS__); \
		stack_trace(); \
	} while (0)

#endif /* CONFIG_DEBUG */

extern bool silent;

extern void panic_printf(const char *fmt, ...) __attribute__((noreturn));

#endif

/** @}
 */
