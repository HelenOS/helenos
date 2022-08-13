/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
