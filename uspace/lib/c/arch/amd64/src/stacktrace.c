/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcamd64 amd64
 * @ingroup lc
 * @{
 */
/** @file
 */

#include <stdint.h>
#include <stdbool.h>

#include <stacktrace.h>

#define FRAME_OFFSET_FP_PREV	0
#define FRAME_OFFSET_RA		8

bool stacktrace_fp_valid(stacktrace_t *st, uintptr_t fp)
{
	(void) st;
	return fp != 0;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_FP_PREV, prev);
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_RA, ra);
}

/** @}
 */
