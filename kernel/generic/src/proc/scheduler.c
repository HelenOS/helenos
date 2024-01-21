/*
 * Copyright (c) 2010 Jakub Jermar
 * Copyright (c) 2023 Jiří Zárevúcky
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

/** @addtogroup kernel_generic_proc
 * @{
 */

/**
 * @file
 * @brief Scheduler and load balancing.
 *
 * This file contains the scheduler and kcpulb kernel thread which
 * performs load-balancing of per-CPU run queues.
 */

#include <assert.h>
#include <atomic.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <time/timeout.h>
#include <time/delay.h>
#include <arch/asm.h>
#include <arch/cycle.h>
#include <atomic.h>
#include <synch/spinlock.h>
#include <config.h>
#include <context.h>
#include <fpu_context.h>
#include <halt.h>
#include <arch.h>
#include <adt/list.h>
#include <panic.h>
#include <cpu.h>
#include <stdio.h>
#include <log.h>
#include <stacktrace.h>

atomic_size_t nrdy;  /**< Number of ready threads in the system. */

#ifdef CONFIG_FPU_LAZY
void scheduler_fpu_lazy_request(void)
{
	fpu_enable();

	/* We need this lock to ensure synchronization with thread destructor. */
	irq_spinlock_lock(&CPU->fpu_lock, false);

	/* Save old context */
	thread_t *owner = atomic_load_explicit(&CPU->fpu_owner, memory_order_relaxed);
	if (owner != NULL) {
		fpu_context_save(&owner->fpu_context);
		atomic_store_explicit(&CPU->fpu_owner, NULL, memory_order_relaxed);
	}

	irq_spinlock_unlock(&CPU->fpu_lock, false);

	if (THREAD->fpu_context_exists) {
		fpu_context_restore(&THREAD->fpu_context);
	} else {
		fpu_init();
		THREAD->fpu_context_exists = true;
	}

	atomic_store_explicit(&CPU->fpu_owner, THREAD, memory_order_relaxed);
}
#endif /* CONFIG_FPU_LAZY */

/** Initialize scheduler
 *
 * Initialize kernel scheduler.
 *
 */
void scheduler_init(void)
{
}

/** Get thread to be scheduled
 *
 * Get the optimal thread to be scheduled
 * according to thread accounting and scheduler
 * policy.
 *
 * @return Thread to be scheduled.
 *
 */
static thread_t *try_find_thread(int *rq_index)
{
	assert(interrupts_disabled());
	assert(CPU != NULL);

	if (atomic_load(&CPU->nrdy) == 0)
		return NULL;

	for (int i = 0; i < RQ_COUNT; i++) {
		irq_spinlock_lock(&(CPU->rq[i].lock), false);
		if (CPU->rq[i].n == 0) {
			/*
			 * If this queue is empty, try a lower-priority queue.
			 */
			irq_spinlock_unlock(&(CPU->rq[i].lock), false);
			continue;
		}

		atomic_dec(&CPU->nrdy);
		atomic_dec(&nrdy);
		CPU->rq[i].n--;

		/*
		 * Take the first thread from the queue.
		 */
		thread_t *thread = list_get_instance(
		    list_first(&CPU->rq[i].rq), thread_t, rq_link);
		list_remove(&thread->rq_link);

		irq_spinlock_unlock(&(CPU->rq[i].lock), false);

		*rq_index = i;
		return thread;
	}

	return NULL;
}

/** Get thread to be scheduled
 *
 * Get the optimal thread to be scheduled
 * according to thread accounting and scheduler
 * policy.
 *
 * @return Thread to be scheduled.
 *
 */
static thread_t *find_best_thread(int *rq_index)
{
	assert(interrupts_disabled());
	assert(CPU != NULL);

	while (true) {
		thread_t *thread = try_find_thread(rq_index);

		if (thread != NULL)
			return thread;

		/*
		 * For there was nothing to run, the CPU goes to sleep
		 * until a hardware interrupt or an IPI comes.
		 * This improves energy saving and hyperthreading.
		 */
		CPU_LOCAL->idle = true;

		/*
		 * Go to sleep with interrupts enabled.
		 * Ideally, this should be atomic, but this is not guaranteed on
		 * all platforms yet, so it is possible we will go sleep when
		 * a thread has just become available.
		 */
		cpu_interruptible_sleep();
	}
}

static void switch_task(task_t *task)
{
	/* If the task stays the same, a lot of work is avoided. */
	if (TASK == task)
		return;

	as_t *old_as = AS;
	as_t *new_as = task->as;

	/* It is possible for two tasks to share one address space. */
	if (old_as != new_as)
		as_switch(old_as, new_as);

	if (TASK)
		task_release(TASK);

	TASK = task;

	task_hold(TASK);

	before_task_runs_arch();
}

/** Prevent rq starvation
 *
 * Prevent low priority threads from starving in rq's.
 *
 * When the function decides to relink rq's, it reconnects
 * respective pointers so that in result threads with 'pri'
 * greater or equal start are moved to a higher-priority queue.
 *
 * @param start Threshold priority.
 *
 */
static void relink_rq(int start)
{
	assert(interrupts_disabled());

	if (CPU_LOCAL->current_clock_tick < CPU_LOCAL->relink_deadline)
		return;

	CPU_LOCAL->relink_deadline = CPU_LOCAL->current_clock_tick + NEEDS_RELINK_MAX;

	/* Temporary cache for lists we are moving. */
	list_t list;
	list_initialize(&list);

	size_t n = 0;

	/* Move every list (except the one with highest priority) one level up. */
	for (int i = RQ_COUNT - 1; i > start; i--) {
		irq_spinlock_lock(&CPU->rq[i].lock, false);

		/* Swap lists. */
		list_swap(&CPU->rq[i].rq, &list);

		/* Swap number of items. */
		size_t tmpn = CPU->rq[i].n;
		CPU->rq[i].n = n;
		n = tmpn;

		irq_spinlock_unlock(&CPU->rq[i].lock, false);
	}

	/* Append the contents of rq[start + 1]  to rq[start]. */
	if (n != 0) {
		irq_spinlock_lock(&CPU->rq[start].lock, false);
		list_concat(&CPU->rq[start].rq, &list);
		CPU->rq[start].n += n;
		irq_spinlock_unlock(&CPU->rq[start].lock, false);
	}
}

/**
 * Do whatever needs to be done with current FPU state before we switch to
 * another thread.
 */
static void fpu_cleanup(void)
{
#if (defined CONFIG_FPU) && (!defined CONFIG_FPU_LAZY)
	fpu_context_save(&THREAD->fpu_context);
#endif
}

/**
 * Set correct FPU state for this thread after switch from another thread.
 */
static void fpu_restore(void)
{
#ifdef CONFIG_FPU_LAZY
	/*
	 * The only concurrent modification possible for fpu_owner here is
	 * another thread changing it from itself to NULL in its destructor.
	 */
	thread_t *owner = atomic_load_explicit(&CPU->fpu_owner,
	    memory_order_relaxed);

	if (THREAD == owner)
		fpu_enable();
	else
		fpu_disable();

#elif defined CONFIG_FPU
	fpu_enable();
	if (THREAD->fpu_context_exists)
		fpu_context_restore(&THREAD->fpu_context);
	else {
		fpu_init();
		THREAD->fpu_context_exists = true;
	}
#endif
}

/** Things to do before we switch to THREAD context.
 */
static void prepare_to_run_thread(int rq_index)
{
	relink_rq(rq_index);

	switch_task(THREAD->task);

	assert(atomic_get_unordered(&THREAD->cpu) == CPU);

	atomic_set_unordered(&THREAD->state, Running);
	atomic_set_unordered(&THREAD->priority, rq_index);  /* Correct rq index */

	/*
	 * Clear the stolen flag so that it can be migrated
	 * when load balancing needs emerge.
	 */
	THREAD->stolen = false;

#ifdef SCHEDULER_VERBOSE
	log(LF_OTHER, LVL_DEBUG,
	    "cpu%u: tid %" PRIu64 " (priority=%d, ticks=%" PRIu64
	    ", nrdy=%zu)", CPU->id, THREAD->tid, rq_index,
	    THREAD->ticks, atomic_load(&CPU->nrdy));
#endif

	/*
	 * Some architectures provide late kernel PA2KA(identity)
	 * mapping in a page fault handler. However, the page fault
	 * handler uses the kernel stack of the running thread and
	 * therefore cannot be used to map it. The kernel stack, if
	 * necessary, is to be mapped in before_thread_runs(). This
	 * function must be executed before the switch to the new stack.
	 */
	before_thread_runs_arch();

#ifdef CONFIG_UDEBUG
	if (atomic_get_unordered(&THREAD->btrace)) {
		istate_t *istate = THREAD->udebug.uspace_state;
		if (istate != NULL) {
			printf("Thread %" PRIu64 " stack trace:\n", THREAD->tid);
			stack_trace_istate(istate);
		} else {
			printf("Thread %" PRIu64 " interrupt state not available\n", THREAD->tid);
		}

		atomic_set_unordered(&THREAD->btrace, false);
	}
#endif

	fpu_restore();

	/* Time allocation in microseconds. */
	uint64_t time_to_run = (rq_index + 1) * 10000;

	/* Set the time of next preemption. */
	CPU_LOCAL->preempt_deadline =
	    CPU_LOCAL->current_clock_tick + us2ticks(time_to_run);

	/* Save current CPU cycle */
	THREAD->last_cycle = get_cycle();
}

static void add_to_rq(thread_t *thread, cpu_t *cpu, int i)
{
	/* Add to the appropriate runqueue. */
	runq_t *rq = &cpu->rq[i];

	irq_spinlock_lock(&rq->lock, false);
	list_append(&thread->rq_link, &rq->rq);
	rq->n++;
	irq_spinlock_unlock(&rq->lock, false);

	atomic_inc(&nrdy);
	atomic_inc(&cpu->nrdy);
}

/** Requeue a thread that was just preempted on this CPU.
 */
static void thread_requeue_preempted(thread_t *thread)
{
	assert(interrupts_disabled());
	assert(atomic_get_unordered(&thread->state) == Running);
	assert(atomic_get_unordered(&thread->cpu) == CPU);

	int prio = atomic_get_unordered(&thread->priority);

	if (prio < RQ_COUNT - 1) {
		prio++;
		atomic_set_unordered(&thread->priority, prio);
	}

	atomic_set_unordered(&thread->state, Ready);

	add_to_rq(thread, CPU, prio);
}

void thread_requeue_sleeping(thread_t *thread)
{
	ipl_t ipl = interrupts_disable();

	assert(atomic_get_unordered(&thread->state) == Sleeping || atomic_get_unordered(&thread->state) == Entering);

	atomic_set_unordered(&thread->priority, 0);
	atomic_set_unordered(&thread->state, Ready);

	/* Prefer the CPU on which the thread ran last */
	cpu_t *cpu = atomic_get_unordered(&thread->cpu);

	if (!cpu) {
		cpu = CPU;
		atomic_set_unordered(&thread->cpu, CPU);
	}

	add_to_rq(thread, cpu, 0);

	interrupts_restore(ipl);
}

static void cleanup_after_thread(thread_t *thread)
{
	assert(CURRENT->mutex_locks == 0);
	assert(interrupts_disabled());

	int expected;

	switch (atomic_get_unordered(&thread->state)) {
	case Running:
		thread_requeue_preempted(thread);
		break;

	case Exiting:
		waitq_close(&thread->join_wq);

		/*
		 * Release the reference CPU has for the thread.
		 * If there are no other references (e.g. threads calling join),
		 * the thread structure is deallocated.
		 */
		thread_put(thread);
		break;

	case Sleeping:
		expected = SLEEP_INITIAL;

		/* Only set SLEEP_ASLEEP in sleep pad if it's still in initial state */
		if (!atomic_compare_exchange_strong_explicit(&thread->sleep_state,
		    &expected, SLEEP_ASLEEP,
		    memory_order_acq_rel, memory_order_acquire)) {

			assert(expected == SLEEP_WOKE);
			/* The thread has already been woken up, requeue immediately. */
			thread_requeue_sleeping(thread);
		}
		break;

	default:
		/*
		 * Entering state is unexpected.
		 */
		panic("tid%" PRIu64 ": unexpected state %s.",
		    thread->tid, thread_states[atomic_get_unordered(&thread->state)]);
		break;
	}
}

/** Switch to scheduler context to let other threads run. */
void scheduler_enter(state_t new_state)
{
	ipl_t ipl = interrupts_disable();

	assert(CPU != NULL);
	assert(THREAD != NULL);

	if (atomic_load(&haltstate))
		halt();

	/* Check if we have a thread to switch to. */

	int rq_index;
	thread_t *new_thread = try_find_thread(&rq_index);

	if (new_thread == NULL && new_state == Running) {
		/* No other thread to run, but we still have work to do here. */
		interrupts_restore(ipl);
		return;
	}

	atomic_set_unordered(&THREAD->state, new_state);

	/* Update thread kernel accounting */
	atomic_time_increment(&THREAD->kcycles, get_cycle() - THREAD->last_cycle);

	fpu_cleanup();

	/*
	 * On Sparc, this saves some extra userspace state that's not
	 * covered by context_save()/context_restore().
	 */
	after_thread_ran_arch();

	if (new_thread) {
		thread_t *old_thread = THREAD;
		CPU_LOCAL->prev_thread = old_thread;
		THREAD = new_thread;
		/* No waiting necessary, we can switch to the new thread directly. */
		prepare_to_run_thread(rq_index);

		current_copy(CURRENT, (current_t *) new_thread->kstack);
		context_swap(&old_thread->saved_context, &new_thread->saved_context);
	} else {
		/*
		 * A new thread isn't immediately available, switch to a separate
		 * stack to sleep or do other idle stuff.
		 */
		current_copy(CURRENT, (current_t *) CPU_LOCAL->stack);
		context_swap(&THREAD->saved_context, &CPU_LOCAL->scheduler_context);
	}

	assert(CURRENT->mutex_locks == 0);
	assert(interrupts_disabled());

	/* Check if we need to clean up after another thread. */
	if (CPU_LOCAL->prev_thread) {
		cleanup_after_thread(CPU_LOCAL->prev_thread);
		CPU_LOCAL->prev_thread = NULL;
	}

	interrupts_restore(ipl);
}

/** Enter main scheduler loop. Never returns.
 *
 * This function switches to a runnable thread as soon as one is available,
 * after which it is only switched back to if a thread is stopping and there is
 * no other thread to run in its place. We need a separate context for that
 * because we're going to block the CPU, which means we need another context
 * to clean up after the previous thread.
 */
void scheduler_run(void)
{
	assert(interrupts_disabled());

	assert(CPU != NULL);
	assert(TASK == NULL);
	assert(THREAD == NULL);
	assert(interrupts_disabled());

	while (!atomic_load(&haltstate)) {
		assert(CURRENT->mutex_locks == 0);

		int rq_index;
		THREAD = find_best_thread(&rq_index);
		prepare_to_run_thread(rq_index);

		/*
		 * Copy the knowledge of CPU, TASK, THREAD and preemption counter to
		 * thread's stack.
		 */
		current_copy(CURRENT, (current_t *) THREAD->kstack);

		/* Switch to thread context. */
		context_swap(&CPU_LOCAL->scheduler_context, &THREAD->saved_context);

		/* Back from another thread. */
		assert(CPU != NULL);
		assert(THREAD != NULL);
		assert(CURRENT->mutex_locks == 0);
		assert(interrupts_disabled());

		cleanup_after_thread(THREAD);

		/*
		 * Necessary because we're allowing interrupts in find_best_thread(),
		 * so we need to avoid other code referencing the thread we left.
		 */
		THREAD = NULL;
	}

	halt();
}

/** Thread wrapper.
 *
 * This wrapper is provided to ensure that a starting thread properly handles
 * everything it needs to do when first scheduled, and when it exits.
 */
void thread_main_func(void)
{
	assert(interrupts_disabled());

	void (*f)(void *) = THREAD->thread_code;
	void *arg = THREAD->thread_arg;

	/* This is where each thread wakes up after its creation */

	/* Check if we need to clean up after another thread. */
	if (CPU_LOCAL->prev_thread) {
		cleanup_after_thread(CPU_LOCAL->prev_thread);
		CPU_LOCAL->prev_thread = NULL;
	}

	interrupts_enable();

	f(arg);

	thread_exit();

	/* Not reached */
}

#ifdef CONFIG_SMP

static thread_t *steal_thread_from(cpu_t *old_cpu, int i)
{
	runq_t *old_rq = &old_cpu->rq[i];
	runq_t *new_rq = &CPU->rq[i];

	ipl_t ipl = interrupts_disable();

	irq_spinlock_lock(&old_rq->lock, false);

	/*
	 * If fpu_owner is any thread in the list, its store is seen here thanks to
	 * the runqueue lock.
	 */
	thread_t *fpu_owner = atomic_load_explicit(&old_cpu->fpu_owner,
	    memory_order_relaxed);

	/* Search rq from the back */
	list_foreach_rev(old_rq->rq, rq_link, thread_t, thread) {

		/*
		 * Do not steal CPU-wired threads, threads
		 * already stolen, threads for which migration
		 * was temporarily disabled or threads whose
		 * FPU context is still in the CPU.
		 */
		if (thread->stolen || thread->nomigrate || thread == fpu_owner) {
			continue;
		}

		thread->stolen = true;
		atomic_set_unordered(&thread->cpu, CPU);

		/*
		 * Ready thread on local CPU
		 */

#ifdef KCPULB_VERBOSE
		log(LF_OTHER, LVL_DEBUG,
		    "kcpulb%u: TID %" PRIu64 " -> cpu%u, "
		    "nrdy=%ld, avg=%ld", CPU->id, thread->tid,
		    CPU->id, atomic_load(&CPU->nrdy),
		    atomic_load(&nrdy) / config.cpu_active);
#endif

		/* Remove thread from ready queue. */
		old_rq->n--;
		list_remove(&thread->rq_link);
		irq_spinlock_unlock(&old_rq->lock, false);

		/* Append thread to local queue. */
		irq_spinlock_lock(&new_rq->lock, false);
		list_append(&thread->rq_link, &new_rq->rq);
		new_rq->n++;
		irq_spinlock_unlock(&new_rq->lock, false);

		atomic_dec(&old_cpu->nrdy);
		atomic_inc(&CPU->nrdy);
		interrupts_restore(ipl);
		return thread;
	}

	irq_spinlock_unlock(&old_rq->lock, false);
	interrupts_restore(ipl);
	return NULL;
}

/** Load balancing thread
 *
 * SMP load balancing thread, supervising thread supplies
 * for the CPU it's wired to.
 *
 * @param arg Generic thread argument (unused).
 *
 */
void kcpulb(void *arg)
{
	size_t average;
	size_t rdy;

loop:
	/*
	 * Work in 1s intervals.
	 */
	thread_sleep(1);

not_satisfied:
	/*
	 * Calculate the number of threads that will be migrated/stolen from
	 * other CPU's. Note that situation can have changed between two
	 * passes. Each time get the most up to date counts.
	 *
	 */
	average = atomic_load(&nrdy) / config.cpu_active + 1;
	rdy = atomic_load(&CPU->nrdy);

	if (average <= rdy)
		goto satisfied;

	size_t count = average - rdy;

	/*
	 * Searching least priority queues on all CPU's first and most priority
	 * queues on all CPU's last.
	 */
	size_t acpu;
	int rq;

	for (rq = RQ_COUNT - 1; rq >= 0; rq--) {
		for (acpu = 0; acpu < config.cpu_active; acpu++) {
			cpu_t *cpu = &cpus[acpu];

			/*
			 * Not interested in ourselves.
			 * Doesn't require interrupt disabling for kcpulb has
			 * THREAD_FLAG_WIRED.
			 *
			 */
			if (CPU == cpu)
				continue;

			if (atomic_load(&cpu->nrdy) <= average)
				continue;

			if (steal_thread_from(cpu, rq) && --count == 0)
				goto satisfied;
		}
	}

	if (atomic_load(&CPU->nrdy)) {
		/*
		 * Be a little bit light-weight and let migrated threads run.
		 *
		 */
		thread_yield();
	} else {
		/*
		 * We failed to migrate a single thread.
		 * Give up this turn.
		 *
		 */
		goto loop;
	}

	goto not_satisfied;

satisfied:
	goto loop;
}
#endif /* CONFIG_SMP */

/** Print information about threads & scheduler queues
 *
 */
void sched_print_list(void)
{
	size_t cpu;
	for (cpu = 0; cpu < config.cpu_count; cpu++) {
		if (!cpus[cpu].active)
			continue;

		printf("cpu%u: address=%p, nrdy=%zu\n",
		    cpus[cpu].id, &cpus[cpu], atomic_load(&cpus[cpu].nrdy));

		unsigned int i;
		for (i = 0; i < RQ_COUNT; i++) {
			irq_spinlock_lock(&(cpus[cpu].rq[i].lock), false);
			if (cpus[cpu].rq[i].n == 0) {
				irq_spinlock_unlock(&(cpus[cpu].rq[i].lock), false);
				continue;
			}

			printf("\trq[%u]: ", i);
			list_foreach(cpus[cpu].rq[i].rq, rq_link, thread_t,
			    thread) {
				printf("%" PRIu64 "(%s) ", thread->tid,
				    thread_states[atomic_get_unordered(&thread->state)]);
			}
			printf("\n");

			irq_spinlock_unlock(&(cpus[cpu].rq[i].lock), false);
		}
	}
}

/** @}
 */
