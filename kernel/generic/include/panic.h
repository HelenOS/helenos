/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_PANIC_H_
#define KERN_PANIC_H_

#include <stdbool.h>
#include <stddef.h>
#include <typedefs.h>
#include <print.h>

#define panic(fmt, ...) \
	panic_common(PANIC_OTHER, NULL, 0, 0, fmt, ##__VA_ARGS__)

#define panic_assert(fmt, ...) \
	panic_common(PANIC_ASSERT, NULL, 0, 0, fmt, ##__VA_ARGS__)

#define panic_badtrap(istate, n, fmt, ...) \
	panic_common(PANIC_BADTRAP, istate, 0, n, fmt, ##__VA_ARGS__)

#define panic_memtrap(istate, access, addr, fmt, ...) \
	panic_common(PANIC_MEMTRAP, istate, access, addr, fmt, ##__VA_ARGS__)

#define unreachable() \
	panic_assert("%s() at %s:%u:\nUnreachable line reached.", \
	    __func__, __FILE__, __LINE__);

typedef enum {
	PANIC_OTHER,
	PANIC_ASSERT,
	PANIC_BADTRAP,
	PANIC_MEMTRAP
} panic_category_t;

struct istate;

extern bool console_override;

extern void panic_common(panic_category_t, struct istate *, int,
    uintptr_t, const char *, ...) __attribute__((noreturn))
    _HELENOS_PRINTF_ATTRIBUTE(5, 6);

#endif

/** @}
 */
