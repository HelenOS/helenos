/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_ARCH_H_
#define KERN_ARCH_H_

#include <config.h>

/** Return the current_t structure
 *
 * The current_t structure holds pointers to various parts of the current
 * execution state, like running task, thread, address space, etc.
 *
 * The current_t structure is located at the base address of the current
 * stack. The stack is assumed to be STACK_SIZE bytes long. The stack base
 * address must be aligned to STACK_SIZE.
 *
 */
#define CURRENT \
	((current_t *) (((uintptr_t) __builtin_frame_address(0)) & \
	    (~((uintptr_t) STACK_SIZE - 1))))

#define MAGIC  UINT32_C(0xfacefeed)

#define container_check(ctn1, ctn2)  ((ctn1) == (ctn2))

#define DEFAULT_CONTAINER  0
#define CONTAINER \
	((CURRENT->task) ? (CURRENT->task->container) : (DEFAULT_CONTAINER))

/* Fwd decl. to avoid include hell. */
struct thread;
struct task;
struct cpu;
struct as;

/** Current structure
 *
 * For each possible kernel stack, structure
 * of the following type will be placed at
 * the base address of the stack.
 *
 */
typedef struct {
	size_t preemption;      /**< Preemption disabled counter and flag. */
	size_t mutex_locks;
	struct thread *thread;  /**< Current thread. */
	struct task *task;      /**< Current task. */
	struct cpu *cpu;        /**< Executing CPU. */
	struct as *as;          /**< Current address space. */
	uint32_t magic;         /**< Magic value. */
} current_t;

typedef struct {
	void (*pre_mm_init)(void);
	void (*post_mm_init)(void);
	void (*post_cpu_init)(void);
	void (*pre_smp_init)(void);
	void (*post_smp_init)(void);
} arch_ops_t;

extern arch_ops_t *arch_ops;

#define ARCH_STRUCT_OP(s, op) \
	do { \
		if ((s)->op) \
			(s)->op(); \
	} while (0)

#define ARCH_OP(op)  ARCH_STRUCT_OP(arch_ops, op)

extern void current_initialize(current_t *);
extern void current_copy(current_t *, current_t *);

extern void calibrate_delay_loop(void);

extern void reboot(void);
extern void arch_reboot(void);
extern void *arch_construct_function(fncptr_t *, void *, void *);

#endif

/** @}
 */
