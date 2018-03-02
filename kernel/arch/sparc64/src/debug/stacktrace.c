/*
 * Copyright (c) 2010 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup sparc64
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

bool uspace_return_address_get(stack_trace_context_t *ctx , uintptr_t *ra)
{
	return false;
}

/** @}
 */
