/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdbool.h>
#include <syscall/copy.h>
#include <typedefs.h>

#define FRAME_OFFSET_FP_PREV  0
#define FRAME_OFFSET_RA       1

bool kernel_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return ctx->fp != 0;
}

bool kernel_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	uint64_t *stack = (void *) ctx->fp;
	*prev = stack[FRAME_OFFSET_FP_PREV];

	return true;
}

bool kernel_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	uint64_t *stack = (void *) ctx->fp;
	*ra = stack[FRAME_OFFSET_RA];

	return true;
}

bool uspace_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return ctx->fp != 0;
}

bool uspace_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return !copy_from_uspace(prev,
	    ctx->fp + sizeof(uintptr_t) * FRAME_OFFSET_FP_PREV, sizeof(*prev));
}

bool uspace_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return !copy_from_uspace(ra,
	    ctx->fp + sizeof(uintptr_t) * FRAME_OFFSET_RA, sizeof(*ra));
}

/** @}
 */
