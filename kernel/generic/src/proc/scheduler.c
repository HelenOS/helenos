/*
 * Copyright (c) 2010 Jakub Jermar
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
#include <arch/faddr.h>
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

static void scheduler_separated_stack(void);

atomic_size_t nrdy;  /**< Number of ready threads in the system. */

/** Take actions before new thread runs.
 *
 * Perform actions that need to be
 * taken before the newly selected
 * thread is passed control.
 *
 * THREAD->lock is locked on entry
 *
 */
static void before_thread_runs(void)
{
	before_thread_runs_arch();

#ifdef CONFIG_FPU_LAZY
	irq_spinlock_lock(&CPU->fpu_lock, true);

	if (THREAD == CPU->fpu_owner)
		fpu_enable();
	else
		fpu_disable();

	irq_spinlock_unlock(&CPU->fpu_lock, true);
#elif defined CONFIG_FPU
	fpu_enable();
	if (THREAD->fpu_context_exists)
		fpu_context_restore(&THREAD->fpu_context);
	else {
		fpu_init();
		THREAD->fpu_context_exists = true;
	}
#endif

#ifdef CONFIG_UDEBUG
	if (THREAD->btrace) {
		istate_t *istate = THREAD->udebug.uspace_state;
		if (istate != NULL) {
			printf("Thread %" PRIu64 " stack trace:\n", THREAD->tid);
			stack_trace_istate(istate);
		}

		THREAD->btrace = false;
	}
#endif
}

/** Take actions after THREAD had run.
 *
 * Perform actions that need to be
 * taken after the running thread
 * had been preempted by the scheduler.
 *
 * THREAD->lock is locked on entry
 *
 */
static void after_thread_ran(void)
{
	after_thread_ran_arch();
}

#ifdef CONFIG_FPU_LAZY
void scheduler_fpu_lazy_request(void)
{
	fpu_enable();
	irq_spinlock_lock(&CPU->fpu_lock, false);

	/* Save old context */
	if (CPU->fpu_owner != NULL) {
		fpu_context_save(&CPU->fpu_owner->fpu_context);
		CPU->fpu_owner = NULL;
	}

	if (THREAD->fpu_context_exists) {
		fpu_context_restore(&THREAD->fpu_context);
	} else {
		fpu_init();
		THREAD->fpu_context_exists = true;
	}

	CPU->fpu_owner = THREAD;

	irq_spinlock_unlock(&CPU->fpu_lock, false);
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
static thread_t *find_best_thread(void)
{
	assert(CPU != NULL);

loop:
	if (atomic_load(&CPU->nrdy) == 0) {
		/*
		 * For there was nothing to run, the CPU goes to sleep
		 * until a hardware interrupt or an IPI comes.
		 * This improves energy saving and hyperthreading.
		 */
		CPU->idle = true;

		/*
		 * Go to sleep with interrupts enabled.
		 * Ideally, this should be atomic, but this is not guaranteed on
		 * all platforms yet, so it is possible we will go sleep when
		 * a thread has just become available.
		 */
		cpu_interruptible_sleep();

		/* Interrupts are disabled again. */
		goto loop;
	}

	assert(!CPU->idle);

	unsigned int i;
	for (i = 0; i < RQ_COUNT; i++) {
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

		irq_spinlock_pass(&(CPU->rq[i].lock), &thread->lock);

		thread->cpu = CPU;
		thread->priority = i;  /* Correct rq index */

		/* Time allocation in microseconds. */
		uint64_t time_to_run = (i + 1) * 10000;

		/* This is safe because interrupts are disabled. */
		CPU->preempt_deadline = CPU->current_clock_tick + us2ticks(time_to_run);

		/*
		 * Clear the stolen flag so that it can be migrated
		 * when load balancing needs emerge.
		 */
		thread->stolen = false;
		irq_spinlock_unlock(&thread->lock, false);

		return thread;
	}

	goto loop;
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
	if (CPU->current_clock_tick < CPU->relink_deadline)
		return;

	CPU->relink_deadline = CPU->current_clock_tick + NEEDS_RELINK_MAX;

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

void scheduler(void)
{
	ipl_t ipl = interrupts_disable();

	if (atomic_load(&haltstate))
		halt();

	if (THREAD) {
		irq_spinlock_lock(&THREAD->lock, false);
	}

	scheduler_locked(ipl);
}

/** The scheduler
 *
 * The thread scheduling procedure.
 * Passes control directly to
 * scheduler_separated_stack().
 *
 */
void scheduler_locked(ipl_t ipl)
{
	assert(CPU != NULL);

	if (THREAD) {
		/* Update thread kernel accounting */
		THREAD->kcycles += get_cycle() - THREAD->last_cycle;

#if (defined CONFIG_FPU) && (!defined CONFIG_FPU_LAZY)
		fpu_context_save(&THREAD->fpu_context);
#endif
		if (!context_save(&THREAD->saved_context)) {
			/*
			 * This is the place where threads leave scheduler();
			 */

			/* Save current CPU cycle */
			THREAD->last_cycle = get_cycle();

			irq_spinlock_unlock(&THREAD->lock, false);
			interrupts_restore(THREAD->saved_ipl);

			return;
		}

		/*
		 * Interrupt priority level of preempted thread is recorded
		 * here to facilitate scheduler() invocations from
		 * interrupts_disable()'d code (e.g. waitq_sleep_timeout()).
		 *
		 */
		THREAD->saved_ipl = ipl;
	}

	/*
	 * Through the 'CURRENT' structure, we keep track of THREAD, TASK, CPU, AS
	 * and preemption counter. At this point CURRENT could be coming either
	 * from THREAD's or CPU's stack.
	 *
	 */
	current_copy(CURRENT, (current_t *) CPU->stack);

	/*
	 * We may not keep the old stack.
	 * Reason: If we kept the old stack and got blocked, for instance, in
	 * find_best_thread(), the old thread could get rescheduled by another
	 * CPU and overwrite the part of its own stack that was also used by
	 * the scheduler on this CPU.
	 *
	 * Moreover, we have to bypass the compiler-generated POP sequence
	 * which is fooled by SP being set to the very top of the stack.
	 * Therefore the scheduler() function continues in
	 * scheduler_separated_stack().
	 *
	 */
	context_t ctx;
	context_save(&ctx);
	context_set(&ctx, FADDR(scheduler_separated_stack),
	    (uintptr_t) CPU->stack, STACK_SIZE);
	context_restore(&ctx);

	/* Not reached */
}

/** Scheduler stack switch wrapper
 *
 * Second part of the scheduler() function
 * using new stack. Handling the actual context
 * switch to a new thread.
 *
 */
void scheduler_separated_stack(void)
{
	assert((!THREAD) || (irq_spinlock_locked(&THREAD->lock)));
	assert(CPU != NULL);
	assert(interrupts_disabled());

	if (THREAD) {
		/* Must be run after the switch to scheduler stack */
		after_thread_ran();

		switch (THREAD->state) {
		case Running:
			irq_spinlock_unlock(&THREAD->lock, false);
			thread_ready(THREAD);
			break;

		case Exiting:
			irq_spinlock_unlock(&THREAD->lock, false);
			waitq_close(&THREAD->join_wq);

			/*
			 * Release the reference CPU has for the thread.
			 * If there are no other references (e.g. threads calling join),
			 * the thread structure is deallocated.
			 */
			thread_put(THREAD);
			break;

		case Sleeping:
			/*
			 * Prefer the thread after it's woken up.
			 */
			THREAD->priority = -1;
			irq_spinlock_unlock(&THREAD->lock, false);
			break;

		default:
			/*
			 * Entering state is unexpected.
			 */
			panic("tid%" PRIu64 ": unexpected state %s.",
			    THREAD->tid, thread_states[THREAD->state]);
			break;
		}

		THREAD = NULL;
	}

	THREAD = find_best_thread();

	irq_spinlock_lock(&THREAD->lock, false);
	int priority = THREAD->priority;
	irq_spinlock_unlock(&THREAD->lock, false);

	relink_rq(priority);

	switch_task(THREAD->task);

	irq_spinlock_lock(&THREAD->lock, false);
	THREAD->state = Running;

#ifdef SCHEDULER_VERBOSE
	log(LF_OTHER, LVL_DEBUG,
	    "cpu%u: tid %" PRIu64 " (priority=%d, ticks=%" PRIu64
	    ", nrdy=%zu)", CPU->id, THREAD->tid, THREAD->priority,
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
	before_thread_runs();

	/*
	 * Copy the knowledge of CPU, TASK, THREAD and preemption counter to
	 * thread's stack.
	 */
	current_copy(CURRENT, (current_t *) THREAD->kstack);

	context_restore(&THREAD->saved_context);

	/* Not reached */
}

#ifdef CONFIG_SMP

static inline void fpu_owner_lock(cpu_t *cpu)
{
#ifdef CONFIG_FPU_LAZY
	irq_spinlock_lock(&cpu->fpu_lock, false);
#endif
}

static inline void fpu_owner_unlock(cpu_t *cpu)
{
#ifdef CONFIG_FPU_LAZY
	irq_spinlock_unlock(&cpu->fpu_lock, false);
#endif
}

static inline thread_t *fpu_owner(cpu_t *cpu)
{
#ifdef CONFIG_FPU_LAZY
	assert(irq_spinlock_locked(&cpu->fpu_lock));
	return cpu->fpu_owner;
#else
	return NULL;
#endif
}

static thread_t *steal_thread_from(cpu_t *old_cpu, int i)
{
	runq_t *old_rq = &old_cpu->rq[i];
	runq_t *new_rq = &CPU->rq[i];

	ipl_t ipl = interrupts_disable();

	fpu_owner_lock(old_cpu);
	irq_spinlock_lock(&old_rq->lock, false);

	/* Search rq from the back */
	list_foreach_rev(old_rq->rq, rq_link, thread_t, thread) {

		irq_spinlock_lock(&thread->lock, false);

		/*
		 * Do not steal CPU-wired threads, threads
		 * already stolen, threads for which migration
		 * was temporarily disabled or threads whose
		 * FPU context is still in the CPU.
		 */
		if (thread->stolen || thread->nomigrate ||
		    thread == fpu_owner(old_cpu)) {
			irq_spinlock_unlock(&thread->lock, false);
			continue;
		}

		fpu_owner_unlock(old_cpu);

		thread->stolen = true;
		thread->cpu = CPU;

		irq_spinlock_unlock(&thread->lock, false);

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
	fpu_owner_unlock(old_cpu);
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
		scheduler();
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

		/* Technically a data race, but we don't really care in this case. */
		int needs_relink = cpus[cpu].relink_deadline - cpus[cpu].current_clock_tick;

		printf("cpu%u: address=%p, nrdy=%zu, needs_relink=%d\n",
		    cpus[cpu].id, &cpus[cpu], atomic_load(&cpus[cpu].nrdy),
		    needs_relink);

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
				    thread_states[thread->state]);
			}
			printf("\n");

			irq_spinlock_unlock(&(cpus[cpu].rq[i].lock), false);
		}
	}
}

/** @}
 */
