/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdbool.h>
#include <stdint.h>
#include <syscall/copy.h>
#include <proc/thread.h>

#include <arch.h>
#include <arch/stack.h>
#include <arch/trap/trap_table.h>

#include <arch/istate_struct.h>

#if defined(SUN4V)
#include <arch/sun4v/arch.h>
#endif

#define FRAME_OFFSET_FP_PREV	14
#define FRAME_OFFSET_RA		15

extern void alloc_window_and_flush(void);

bool kernel_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	uintptr_t kstack;

#if defined(SUN4U)
	kstack = read_from_ag_g6();
#elif defined(SUN4V)
	kstack = asi_u64_read(ASI_SCRATCHPAD, SCRATCHPAD_KSTACK);
#endif

	kstack += STACK_BIAS;
	kstack -= ISTATE_SIZE;

	if (THREAD && (ctx->fp == kstack))
		return false;
	return ctx->fp != 0;
}

bool kernel_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	uint64_t *stack = (void *) ctx->fp;
	alloc_window_and_flush();
	*prev = stack[FRAME_OFFSET_FP_PREV] + STACK_BIAS;
	return true;
}

bool kernel_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	uint64_t *stack = (void *) ctx->fp;
	alloc_window_and_flush();
	*ra = stack[FRAME_OFFSET_RA];
	return true;
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
