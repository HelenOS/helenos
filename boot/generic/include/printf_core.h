/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_PRINTF_CORE_H_
#define BOOT_PRINTF_CORE_H_

#include <stddef.h>
#include <stdarg.h>

/** Structure for specifying output methods for different printf clones. */
typedef struct {
	/* String output function, returns number of printed characters or EOF */
	int (*str_write)(const char *, size_t, void *);

	/* User data - output stream specification, state, locks, etc. */
	void *data;
} printf_spec_t;

extern int printf_core(const char *, printf_spec_t *, va_list);

#endif

/** @}
 */
