/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_debug
 * @{
 */
/** @file
 */

#include <stacktrace.h>
#include <stdbool.h>
#include <interrupt.h>
#include <symtab.h>
#include <stdio.h>

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
