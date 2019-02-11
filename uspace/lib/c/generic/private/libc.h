/*
 * Copyright (c) 2011 Martin Decky
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

#ifndef _LIBC_PRIVATE_LIBC_H_
#define _LIBC_PRIVATE_LIBC_H_

#include <types/common.h>

/* Type of the main C function. */
typedef int (*main_fn_t)(int, char **);

/**
 * Used for C++ constructors/destructors
 * and the GCC constructor/destructor extension.
 */
typedef void (*init_array_entry_t)();
typedef void (*fini_array_entry_t)();

typedef struct {
	main_fn_t main;
	const void *elfstart;
	const void *end;
	init_array_entry_t *preinit_array;
	int preinit_array_len;
	init_array_entry_t *init_array;
	int init_array_len;
	fini_array_entry_t *fini_array;
	int fini_array_len;
} progsymbols_t;

extern progsymbols_t __progsymbols;
extern void __libc_main(void *) __attribute__((noreturn));
extern void __libc_exit(int) __attribute__((noreturn));
extern void __libc_abort(void) __attribute__((noreturn));

#endif

/** @}
 */
