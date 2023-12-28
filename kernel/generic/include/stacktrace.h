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

#ifndef KERN_STACKTRACE_H_
#define KERN_STACKTRACE_H_

#include <stdbool.h>
#include <typedefs.h>

/* Forward declaration. */
struct istate;

typedef struct {
	uintptr_t fp;
	uintptr_t pc;
	struct istate *istate;
} stack_trace_context_t;

typedef struct {
	bool (*stack_trace_context_validate)(stack_trace_context_t *);
	bool (*frame_pointer_prev)(stack_trace_context_t *, uintptr_t *);
	bool (*return_address_get)(stack_trace_context_t *, uintptr_t *);
	bool (*symbol_resolve)(uintptr_t, int, const char **, uintptr_t *, const char **, const char **, int *, int *);
} stack_trace_ops_t;

extern stack_trace_ops_t kst_ops;
extern stack_trace_ops_t ust_ops;

extern void stack_trace(void);
extern void stack_trace_istate(struct istate *);
extern void stack_trace_ctx(stack_trace_ops_t *, stack_trace_context_t *);

/*
 * The following interface is to be implemented by each architecture.
 */
extern uintptr_t frame_pointer_get(void);
extern uintptr_t program_counter_get(void);

extern bool kernel_stack_trace_context_validate(stack_trace_context_t *);
extern bool kernel_frame_pointer_prev(stack_trace_context_t *, uintptr_t *);
extern bool kernel_return_address_get(stack_trace_context_t *, uintptr_t *);

extern bool uspace_stack_trace_context_validate(stack_trace_context_t *);
extern bool uspace_frame_pointer_prev(stack_trace_context_t *, uintptr_t *);
extern bool uspace_return_address_get(stack_trace_context_t *, uintptr_t *);

#endif

/** @}
 */
