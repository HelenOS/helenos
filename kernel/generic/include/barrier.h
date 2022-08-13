/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_COMPILER_BARRIER_H_
#define KERN_COMPILER_BARRIER_H_

#include <stdatomic.h>
#include <stddef.h>

static inline void compiler_barrier(void)
{
	atomic_signal_fence(memory_order_seq_cst);
}

static inline void memory_barrier(void)
{
	atomic_thread_fence(memory_order_seq_cst);
}

static inline void read_barrier(void)
{
	atomic_thread_fence(memory_order_acquire);
}

static inline void write_barrier(void)
{
	atomic_thread_fence(memory_order_release);
}

#define CS_ENTER_BARRIER() atomic_thread_fence(memory_order_acquire)
#define CS_LEAVE_BARRIER() atomic_thread_fence(memory_order_release)

/** Forces the compiler to access (ie load/store) the variable only once. */
#define ACCESS_ONCE(var) (*((volatile typeof(var)*)&(var)))

extern void smc_coherence(void *, size_t);

#endif /* KERN_COMPILER_BARRIER_H_ */
