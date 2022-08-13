/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_STACKTRACE_H_
#define _LIBC_STACKTRACE_H_

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	errno_t (*read_uintptr)(void *, uintptr_t, uintptr_t *);
	int (*printf)(const char *, ...);
} stacktrace_ops_t;

typedef struct {
	void *op_arg;
	stacktrace_ops_t *ops;
} stacktrace_t;

extern void stacktrace_print(void);
extern void stacktrace_kio_print(void);
extern void stacktrace_print_fp_pc(uintptr_t, uintptr_t);
extern void stacktrace_print_generic(stacktrace_ops_t *, void *, uintptr_t,
    uintptr_t);

/*
 * The following interface is to be implemented by each architecture.
 */
extern bool stacktrace_fp_valid(stacktrace_t *, uintptr_t);
extern errno_t stacktrace_fp_prev(stacktrace_t *, uintptr_t, uintptr_t *);
extern errno_t stacktrace_ra_get(stacktrace_t *, uintptr_t, uintptr_t *);

extern void stacktrace_prepare(void);
extern uintptr_t stacktrace_fp_get(void);
extern uintptr_t stacktrace_pc_get(void);

#endif

/** @}
 */
