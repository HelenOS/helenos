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
	bool (*symbol_resolve)(uintptr_t, const char **, uintptr_t *);
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
