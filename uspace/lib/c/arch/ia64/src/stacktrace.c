/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia64 ia64
 * @ingroup lc
 * @{
 */
/** @file
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <stacktrace.h>

bool stacktrace_fp_valid(stacktrace_t *st, uintptr_t fp)
{
	(void) st;
	(void) fp;
	return false;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	(void) st;
	(void) fp;
	(void) prev;
	return ENOTSUP;
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	(void) st;
	(void) fp;
	(void) ra;
	return ENOTSUP;
}

/** @}
 */
