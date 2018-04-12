/*
 * Copyright (c) 2012 Adam Hraska
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

/**
 * @file
 * @brief Facility to invoke functions on other cpus via IPIs.
 */

#include <smp/smp_call.h>
#include <arch/barrier.h>
#include <arch/asm.h>  /* interrupt_disable */
#include <arch.h>
#include <assert.h>
#include <config.h>
#include <preemption.h>
#include <cpu.h>

static void call_start(smp_call_t *call_info, smp_call_func_t func, void *arg);
static void call_done(smp_call_t *call_info);
static void call_wait(smp_call_t *call_info);


/** Init smp_call() on the local cpu. */
void smp_call_init(void)
{
	assert(CPU);
	assert(PREEMPTION_DISABLED || interrupts_disabled());

	spinlock_initialize(&CPU->smp_calls_lock, "cpu[].smp_calls_lock");
	list_initialize(&CPU->smp_pending_calls);
}

/** Invokes a function on a specific cpu and waits for it to complete.
 *
 * Calls @a func on the CPU denoted by its logical id @cpu_id .
 * The function will execute with interrupts disabled. It should
 * be a quick and simple function and must never block.
 *
 * If @a cpu_id is the local CPU, the function will be invoked
 * directly.
 *
 * All memory accesses of prior to smp_call() will be visible
 * to @a func on cpu @a cpu_id. Similarly, any changes @a func
 * makes on cpu @a cpu_id will be visible on this cpu once
 * smp_call() returns.
 *
 * Invoking @a func on the destination cpu acts as a memory barrier
 * on that cpu.
 *
 * @param cpu_id Destination CPU's logical id (eg CPU->id)
 * @param func Function to call.
 * @param arg Argument to pass to the user supplied function @a func.
 */
void smp_call(unsigned int cpu_id, smp_call_func_t func, void *arg)
{
	smp_call_t call_info;
	smp_call_async(cpu_id, func, arg, &call_info);
	smp_call_wait(&call_info);
}

/** Invokes a function on a specific cpu asynchronously.
 *
 * Calls @a func on the CPU denoted by its logical id @cpu_id .
 * The function will execute with interrupts disabled. It should
 * be a quick and simple function and must never block.
 *
 * Pass @a call_info to smp_call_wait() in order to wait for
 * @a func to complete.
 *
 * @a call_info must be valid until/after @a func returns. Use
 * smp_call_wait() to wait until it is safe to free @a call_info.
 *
 * If @a cpu_id is the local CPU, the function will be invoked
 * directly. If the destination cpu id @a cpu_id is invalid
 * or denotes an inactive cpu, the call is discarded immediately.
 *
 * All memory accesses of the caller prior to smp_call_async()
 * will be made visible to @a func on the other cpu. Similarly,
 * any changes @a func makes on cpu @a cpu_id will be visible
 * to this cpu when smp_call_wait() returns.
 *
 * Invoking @a func on the destination cpu acts as a memory barrier
 * on that cpu.
 *
 * Interrupts must be enabled. Otherwise you run the risk
 * of a deadlock.
 *
 * @param cpu_id Destination CPU's logical id (eg CPU->id).
 * @param func Function to call.
 * @param arg Argument to pass to the user supplied function @a func.
 * @param call_info Use it to wait for the function to complete. Must
 *          be valid until the function completes.
 */
void smp_call_async(unsigned int cpu_id, smp_call_func_t func, void *arg,
    smp_call_t *call_info)
{
	/*
	 * Interrupts must not be disabled or you run the risk of a deadlock
	 * if both the destination and source cpus try to send an IPI to each
	 * other with interrupts disabled. Because the interrupts are disabled
	 * the IPIs cannot be delivered and both cpus will forever busy wait
	 * for an acknowledgment of the IPI from the other cpu.
	 */
	assert(!interrupts_disabled());
	assert(call_info != NULL);

	/* Discard invalid calls. */
	if (config.cpu_count <= cpu_id || !cpus[cpu_id].active) {
		call_start(call_info, func, arg);
		call_done(call_info);
		return;
	}

	/* Protect cpu->id against migration. */
	preemption_disable();

	call_start(call_info, func, arg);

	if (cpu_id != CPU->id) {
#ifdef CONFIG_SMP
		spinlock_lock(&cpus[cpu_id].smp_calls_lock);
		list_append(&call_info->calls_link, &cpus[cpu_id].smp_pending_calls);
		spinlock_unlock(&cpus[cpu_id].smp_calls_lock);

		/*
		 * If a platform supports SMP it must implement arch_smp_call_ipi().
		 * It should issue an IPI on cpu_id and invoke smp_call_ipi_recv()
		 * on cpu_id in turn.
		 *
		 * Do not implement as just an empty dummy function. Instead
		 * consider providing a full implementation or at least a version
		 * that panics if invoked. Note that smp_call_async() never
		 * calls arch_smp_call_ipi() on uniprocessors even if CONFIG_SMP.
		 */
		arch_smp_call_ipi(cpu_id);
#endif
	} else {
		/* Invoke local smp calls in place. */
		ipl_t ipl = interrupts_disable();
		func(arg);
		interrupts_restore(ipl);

		call_done(call_info);
	}

	preemption_enable();
}

/** Waits for a function invoked on another CPU asynchronously to complete.
 *
 * Does not sleep but rather spins.
 *
 * Example usage:
 * @code
 * void hello(void *p) {
 *     puts((char*)p);
 * }
 *
 * smp_call_t call_info;
 * smp_call_async(cpus[2].id, hello, "hi!\n", &call_info);
 * // Do some work. In the meantime, hello() is executed on cpu2.
 * smp_call_wait(&call_info);
 * @endcode
 *
 * @param call_info Initialized by smp_call_async().
 */
void smp_call_wait(smp_call_t *call_info)
{
	call_wait(call_info);
}

#ifdef CONFIG_SMP

/** Architecture independent smp call IPI handler.
 *
 * Interrupts must be disabled. Tolerates spurious calls.
 */
void smp_call_ipi_recv(void)
{
	assert(interrupts_disabled());
	assert(CPU);

	list_t calls_list;
	list_initialize(&calls_list);

	/*
	 * Acts as a load memory barrier. Any changes made by the cpu that
	 * added the smp_call to calls_list will be made visible to this cpu.
	 */
	spinlock_lock(&CPU->smp_calls_lock);
	list_concat(&calls_list, &CPU->smp_pending_calls);
	spinlock_unlock(&CPU->smp_calls_lock);

	/* Walk the list manually, so that we can safely remove list items. */
	for (link_t *cur = calls_list.head.next, *next = cur->next;
	    !list_empty(&calls_list); cur = next, next = cur->next) {

		smp_call_t *call_info = list_get_instance(cur, smp_call_t, calls_link);
		list_remove(cur);

		call_info->func(call_info->arg);
		call_done(call_info);
	}
}

#endif /* CONFIG_SMP */

static void call_start(smp_call_t *call_info, smp_call_func_t func, void *arg)
{
	link_initialize(&call_info->calls_link);
	call_info->func = func;
	call_info->arg = arg;

	/*
	 * We can't use standard spinlocks here because we want to lock
	 * the structure on one cpu and unlock it on another (without
	 * messing up the preemption count).
	 */
	atomic_set(&call_info->pending, 1);

	/* Let initialization complete before continuing. */
	memory_barrier();
}

static void call_done(smp_call_t *call_info)
{
	/*
	 * Separate memory accesses of the called function from the
	 * announcement of its completion.
	 */
	memory_barrier();
	atomic_set(&call_info->pending, 0);
}

static void call_wait(smp_call_t *call_info)
{
	do {
		/*
		 * Ensure memory accesses following call_wait() are ordered
		 * after completion of the called function on another cpu.
		 * Also, speed up loading of call_info->pending.
		 */
		memory_barrier();
	} while (atomic_get(&call_info->pending));
}


/** @}
 */
