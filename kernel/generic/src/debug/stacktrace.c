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
#include <interrupt.h>
#include <typedefs.h>
#include <symtab.h>

#define STACK_FRAMES_MAX	20

void
stack_trace_fp_pc(stack_trace_ops_t *ops, uintptr_t fp, uintptr_t pc)
{
	int cnt = 0;
	const char *symbol;
	uintptr_t offset;
	
	while (cnt++ < STACK_FRAMES_MAX && ops->frame_pointer_validate(fp)) {
		if (ops->symbol_resolve &&
		    ops->symbol_resolve(pc, &symbol, &offset)) {
		    	if (offset)
				printf("%p: %s+%p()\n", fp, symbol, offset);
			else
				printf("%p: %s()\n", fp, symbol);
		} else {
			printf("%p: %p()\n", fp, pc);
		}
		if (!ops->return_address_get(fp, &pc))
			break;
		if (!ops->frame_pointer_prev(fp, &fp))
			break;
	}
}

void stack_trace(void)
{
	stack_trace_fp_pc(&kst_ops, frame_pointer_get(), program_counter_get());

	/*
	 * Prevent the tail call optimization of the previous call by
	 * making it a non-tail call.
	 */
	(void) frame_pointer_get();
}

void stack_trace_istate(istate_t *istate)
{
	if (istate_from_uspace(istate))
		stack_trace_fp_pc(&ust_ops, istate_get_fp(istate),
		    istate_get_pc(istate));
	else
		stack_trace_fp_pc(&kst_ops, istate_get_fp(istate),
		    istate_get_pc(istate));
}

static bool kernel_symbol_resolve(uintptr_t addr, const char **sp, uintptr_t *op)
{
	return (symtab_name_lookup(addr, sp, op) == 0);
}

stack_trace_ops_t kst_ops = {
	.frame_pointer_validate = kernel_frame_pointer_validate,
	.frame_pointer_prev = kernel_frame_pointer_prev,
	.return_address_get = kernel_return_address_get,
	.symbol_resolve = kernel_symbol_resolve
};

stack_trace_ops_t ust_ops = {
	.frame_pointer_validate = uspace_frame_pointer_validate,
	.frame_pointer_prev = uspace_frame_pointer_prev,
	.return_address_get = uspace_return_address_get,
	.symbol_resolve = NULL
};

/** @}
 */
