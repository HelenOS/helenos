/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRINTF_CORE_H_
#define _LIBC_PRINTF_CORE_H_

#include <stddef.h>
#include <stdarg.h>
#include <uchar.h>

/** Structure for specifying output methods for different printf clones. */
typedef struct {
	/* String output function, returns number of printed characters or EOF */
	int (*str_write)(const char *, size_t, void *);

	/* Wide string output function, returns number of printed characters or EOF */
	int (*wstr_write)(const char32_t *, size_t, void *);

	/* User data - output stream specification, state, locks, etc. */
	void *data;
} printf_spec_t;

extern int printf_core(const char *, printf_spec_t *, va_list);

#endif

/** @}
 */
