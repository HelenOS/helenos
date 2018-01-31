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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_ARCH_H_
#define KERN_ARCH_H_

#include <arch/asm.h>   /* get_stack_base() */
#include <config.h>


/*
 * THE is not an abbreviation, but the English definite article written in
 * capital letters. It means the current pointer to something, e.g. thread,
 * processor or address space. Kind reader of this comment shall appreciate
 * the wit of constructs like THE->thread and similar.
 */
#define THE  ((the_t * )(get_stack_base()))

#define MAGIC                UINT32_C(0xfacefeed)

#define container_check(ctn1, ctn2)  ((ctn1) == (ctn2))

#define DEFAULT_CONTAINER  0
#define CONTAINER \
	((THE->task) ? (THE->task->container) : (DEFAULT_CONTAINER))

/* Fwd decl. to avoid include hell. */
struct thread;
struct task;
struct cpu;
struct as;

/**
 * For each possible kernel stack, structure
 * of the following type will be placed at
 * the base address of the stack.
 */
typedef struct {
	size_t preemption;     /**< Preemption disabled counter and flag. */
#ifdef RCU_PREEMPT_A
	size_t rcu_nesting;    /**< RCU nesting count and flag. */
#endif 
	struct thread *thread; /**< Current thread. */
	struct task *task;     /**< Current task. */
	struct cpu *cpu;       /**< Executing cpu. */
	struct as *as;         /**< Current address space. */
	uint32_t magic;        /**< Magic value */
} the_t;

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

#define ARCH_OP(op)	ARCH_STRUCT_OP(arch_ops, op)

extern void the_initialize(the_t *);
extern void the_copy(the_t *, the_t *);

extern void calibrate_delay_loop(void);

extern void reboot(void);
extern void arch_reboot(void);
extern void *arch_construct_function(fncptr_t *, void *, void *);

#endif

/** @}
 */
