/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
