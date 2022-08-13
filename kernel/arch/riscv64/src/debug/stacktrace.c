/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
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
	return true;
}

bool kernel_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return true;
}

bool kernel_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return true;
}

bool uspace_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return true;
}

bool uspace_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return true;
}

bool uspace_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return true;
}

uintptr_t frame_pointer_get(void)
{
	return 0;
}

uintptr_t program_counter_get(void)
{
	return 0;
}

/** @}
 */
