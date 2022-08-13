/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdint.h>
#include <stdbool.h>
#include <libarch/stack.h>
#include <errno.h>

#include <stacktrace.h>

#define FRAME_OFFSET_FP_PREV	(14 * 8)
#define FRAME_OFFSET_RA		(15 * 8)

bool stacktrace_fp_valid(stacktrace_t *st, uintptr_t fp)
{
	(void) st;
	return fp != 0;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	uintptr_t bprev;
	errno_t rc;

	rc = (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_FP_PREV, &bprev);
	if (rc == EOK)
		*prev = bprev + STACK_BIAS;
	return rc;
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_RA, ra);
}

/** @}
 */
