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

#include <debug/line.h>

#define STACK_FRAMES_MAX  20

void stack_trace_ctx(stack_trace_ops_t *ops, stack_trace_context_t *ctx)
{
	int cnt = 0;

	uintptr_t fp;
	uintptr_t pc;

	while ((cnt++ < STACK_FRAMES_MAX) &&
	    (ops->stack_trace_context_validate(ctx))) {

		const char *symbol = NULL;
		uintptr_t symbol_addr = 0;
		const char *file_name = NULL;
		const char *dir_name = NULL;
		int line = 0;
		int column = 0;

		/*
		 * If this isn't the first frame, move pc back by one byte to read the
		 * position of the call instruction, not the return address.
		 */
		pc = cnt == 1 ? ctx->pc : ctx->pc - 1;

		if (ops->symbol_resolve &&
		    ops->symbol_resolve(pc, 0, &symbol, &symbol_addr, &file_name, &dir_name, &line, &column)) {

			if (symbol == NULL)
				symbol = "<unknown>";

			if (file_name == NULL && line == 0) {
				printf("%p: %24s()+%zu\n", (void *) ctx->fp, symbol, ctx->pc - symbol_addr);
			} else {
				if (file_name == NULL)
					file_name = "<unknown>";
				if (dir_name == NULL)
					dir_name = "<unknown>";

				printf("%p: %20s()+%zu\t %s/%s:%d:%d\n",
				    (void *) ctx->fp, symbol, ctx->pc - symbol_addr,
				    dir_name, file_name, line, column);
			}
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
resolve_kernel_address(uintptr_t addr, int op_index,
    const char **symbol, uintptr_t *symbol_addr,
    const char **filename, const char **dirname,
    int *line, int *column)
{
	*symbol_addr = 0;
	*symbol = symtab_name_lookup(addr, symbol_addr, &kernel_sections);

	return debug_line_get_address_info(&kernel_sections, addr, op_index, filename, dirname, line, column) || *symbol_addr != 0;
}

static bool
resolve_uspace_address(uintptr_t addr, int op_index,
    const char **symbol, uintptr_t *symbol_addr,
    const char **filename, const char **dirname,
    int *line, int *column)
{
	if (TASK->debug_sections == NULL)
		return false;

	debug_sections_t *scs = TASK->debug_sections;

	*symbol_addr = 0;
	*symbol = symtab_name_lookup(addr, symbol_addr, scs);

	return debug_line_get_address_info(scs, addr, op_index, filename, dirname, line, column) || *symbol_addr != 0;
}

stack_trace_ops_t kst_ops = {
	.stack_trace_context_validate = kernel_stack_trace_context_validate,
	.frame_pointer_prev = kernel_frame_pointer_prev,
	.return_address_get = kernel_return_address_get,
	.symbol_resolve = resolve_kernel_address,
};

stack_trace_ops_t ust_ops = {
	.stack_trace_context_validate = uspace_stack_trace_context_validate,
	.frame_pointer_prev = uspace_frame_pointer_prev,
	.return_address_get = uspace_return_address_get,
	.symbol_resolve = resolve_uspace_address,
};

/** @}
 */
