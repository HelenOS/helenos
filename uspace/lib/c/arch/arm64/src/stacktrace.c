/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm64
 * @{
 */
/** @file
 * @brief
 */

#include <stacktrace.h>
#include <stdint.h>
#include <stdbool.h>

#define FRAME_OFFSET_FP_PREV  0
#define FRAME_OFFSET_RA       8

bool stacktrace_fp_valid(stacktrace_t *st __attribute__((unused)),
    uintptr_t fp)
{
	return fp != 0;
}

errno_t stacktrace_fp_prev(stacktrace_t *st, uintptr_t fp, uintptr_t *prev)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_FP_PREV,
	    prev);
}

errno_t stacktrace_ra_get(stacktrace_t *st, uintptr_t fp, uintptr_t *ra)
{
	return (*st->ops->read_uintptr)(st->op_arg, fp + FRAME_OFFSET_RA, ra);
}

/** @}
 */
