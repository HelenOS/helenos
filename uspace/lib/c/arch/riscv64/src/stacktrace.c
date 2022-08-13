/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <stdint.h>
#include <stdbool.h>
#include <stacktrace.h>

bool stacktrace_fp_valid(stacktrace_t *st, uintptr_t fp)
{
	return true;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	return 0;
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	return 0;
}

void stacktrace_prepare(void)
{
}

uintptr_t stacktrace_fp_get(void)
{
	return 0;
}

uintptr_t stacktrace_pc_get(void)
{
	return 0;
}

/** @}
 */
