/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdbool.h>
#include <syscall/copy.h>
#include <typedefs.h>

bool kernel_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return false;
}

bool kernel_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return false;
}

bool kernel_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return false;
}

bool uspace_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return false;
}

bool uspace_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return false;
}

bool uspace_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return false;
}

/** @}
 */
