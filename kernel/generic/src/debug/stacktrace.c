/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup genericdebug
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdbool.h>
#include <interrupt.h>
#include <symtab.h>
#include <print.h>

#define STACK_FRAMES_MAX  20

void stack_trace_ctx(stack_trace_ops_t *ops, stack_trace_context_t *ctx)
{
	int cnt = 0;
	const char *symbol;
	uintptr_t offset;
	uintptr_t fp;
	uintptr_t pc;

	while ((cnt++ < STACK_FRAMES_MAX) &&
	    (ops->stack_trace_context_validate(ctx))) {
		if (ops->symbol_resolve &&
		    ops->symbol_resolve(ctx->pc, &symbol, &offset)) {
			if (offset)
				printf("%p: %s()+%p\n", (void *) ctx->fp,
				    symbol, (void *) offset);
			else
				printf("%p: %s()\n", (void *) ctx->fp, symbol);
		} else
			printf("%p: %p()\n", (void *) ctx->fp, (void *) ctx->pc);

		if (!ops->return_address_get(ctx, &pc))
			break;

		if (!ops->frame_pointer_prev(ctx, &fp))
			break;

		ctx->fp = fp;
		ctx->pc = pc;
	}
}

void stack_trace(void)
{
	stack_trace_context_t ctx = {
		.fp = frame_pointer_get(),
		.pc = program_counter_get(),
		.istate = NULL
	};

	stack_trace_ctx(&kst_ops, &ctx);

	/*
	 * Prevent the tail call optimization of the previous call by
	 * making it a non-tail call.
	 */
	(void) frame_pointer_get();
}

void stack_trace_istate(istate_t *istate)
{
	stack_trace_context_t ctx = {
		.fp = istate_get_fp(istate),
		.pc = istate_get_pc(istate),
		.istate = istate
	};

	if (istate_from_uspace(istate))
		stack_trace_ctx(&ust_ops, &ctx);
	else
		stack_trace_ctx(&kst_ops, &ctx);
}

static bool
kernel_symbol_resolve(uintptr_t addr, const char **sp, uintptr_t *op)
{
	return (symtab_name_lookup(addr, sp, op) == 0);
}

stack_trace_ops_t kst_ops = {
	.stack_trace_context_validate = kernel_stack_trace_context_validate,
	.frame_pointer_prev = kernel_frame_pointer_prev,
	.return_address_get = kernel_return_address_get,
	.symbol_resolve = kernel_symbol_resolve
};

stack_trace_ops_t ust_ops = {
	.stack_trace_context_validate = uspace_stack_trace_context_validate,
	.frame_pointer_prev = uspace_frame_pointer_prev,
	.return_address_get = uspace_return_address_get,
	.symbol_resolve = NULL
};

/** @}
 */
