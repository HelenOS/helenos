/*
 * Copyright (c) 2001-2007 Jakub Jermar
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

/** @addtogroup genericproc
 * @{
 */

/**
 * @file
 * @brief	Scheduler and load balancing.
 *
 * This file contains the scheduler and kcpulb kernel thread which
 * performs load-balancing of per-CPU run queues.
 */

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
#include <func.h>
#include <arch.h>
#include <adt/list.h>
#include <panic.h>
#include <cpu.h>
#include <print.h>
#include <debug.h>

static void before_task_runs(void);
static void before_thread_runs(void);
static void after_thread_ran(void);
static void scheduler_separated_stack(void);

atomic_t nrdy;	/**< Number of ready threads in the system. */

/** Carry out actions before new task runs. */
void before_task_runs(void)
{
	before_task_runs_arch();
}

/** Take actions before new thread runs.
 *
 * Perform actions that need to be
 * taken before the newly selected
 * tread is passed control.
 *
 * THREAD->lock is locked on entry
 *
 */
void before_thread_runs(void)
{
	before_thread_runs_arch();
#ifdef CONFIG_FPU_LAZY
	if(THREAD == CPU->fpu_owner) 
		fpu_enable();
	else
		fpu_disable(); 
#else
	fpu_enable();
	if (THREAD->fpu_context_exists)
		fpu_context_restore(THREAD->saved_fpu_context);
	else {
		fpu_init();
		THREAD->fpu_context_exists = 1;
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
void after_thread_ran(void)
{
	after_thread_ran_arch();
}

#ifdef CONFIG_FPU_LAZY
void scheduler_fpu_lazy_request(void)
{
restart:
	fpu_enable();
	spinlock_lock(&CPU->lock);

	/* Save old context */
	if (CPU->fpu_owner != NULL) {  
		spinlock_lock(&CPU->fpu_owner->lock);
		fpu_context_save(CPU->fpu_owner->saved_fpu_context);
		/* don't prevent migration */
		CPU->fpu_owner->fpu_context_engaged = 0;
		spinlock_unlock(&CPU->fpu_owner->lock);
		CPU->fpu_owner = NULL;
	}

	spinlock_lock(&THREAD->lock);
	if (THREAD->fpu_context_exists) {
		fpu_context_restore(THREAD->saved_fpu_context);
	} else {
		/* Allocate FPU context */
		if (!THREAD->saved_fpu_context) {
			/* Might sleep */
			spinlock_unlock(&THREAD->lock);
			spinlock_unlock(&CPU->lock);
			THREAD->saved_fpu_context =
			    (fpu_context_t *) slab_alloc(fpu_context_slab, 0);
			/* We may have switched CPUs during slab_alloc */
			goto restart; 
		}
		fpu_init();
		THREAD->fpu_context_exists = 1;
	}
	CPU->fpu_owner = THREAD;
	THREAD->fpu_context_engaged = 1;
	spinlock_unlock(&THREAD->lock);

	spinlock_unlock(&CPU->lock);
}
#endif

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
	thread_t *t;
	runq_t *r;
	int i;

	ASSERT(CPU != NULL);

loop:
	
	if (atomic_get(&CPU->nrdy) == 0) {
		/*
		 * For there was nothing to run, the CPU goes to sleep
		 * until a hardware interrupt or an IPI comes.
		 * This improves energy saving and hyperthreading.
		 */

		 /* Mark CPU as it was idle this clock tick */
		 spinlock_lock(&CPU->lock);
		 CPU->idle = true;
		 spinlock_unlock(&CPU->lock);

		 interrupts_enable();
		 /*
		 * An interrupt might occur right now and wake up a thread.
		 * In such case, the CPU will continue to go to sleep
		 * even though there is a runnable thread.
		 */
		 cpu_sleep();
		 interrupts_disable();
		 goto loop;
	}
	
	for (i = 0; i < RQ_COUNT; i++) {
		r = &CPU->rq[i];
		spinlock_lock(&r->lock);
		if (r->n == 0) {
			/*
			 * If this queue is empty, try a lower-priority queue.
			 */
			spinlock_unlock(&r->lock);
			continue;
		}

		atomic_dec(&CPU->nrdy);
		atomic_dec(&nrdy);
		r->n--;

		/*
		 * Take the first thread from the queue.
		 */
		t = list_get_instance(r->rq_head.next, thread_t, rq_link);
		list_remove(&t->rq_link);

		spinlock_unlock(&r->lock);

		spinlock_lock(&t->lock);
		t->cpu = CPU;

		t->ticks = us2ticks((i + 1) * 10000);
		t->priority = i;	/* correct rq index */

		/*
		 * Clear the THREAD_FLAG_STOLEN flag so that t can be migrated
		 * when load balancing needs emerge.
		 */
		t->flags &= ~THREAD_FLAG_STOLEN;
		spinlock_unlock(&t->lock);

		return t;
	}
	goto loop;

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
	link_t head;
	runq_t *r;
	int i, n;

	list_initialize(&head);
	spinlock_lock(&CPU->lock);
	if (CPU->needs_relink > NEEDS_RELINK_MAX) {
		for (i = start; i < RQ_COUNT - 1; i++) {
			/* remember and empty rq[i + 1] */
			r = &CPU->rq[i + 1];
			spinlock_lock(&r->lock);
			list_concat(&head, &r->rq_head);
			n = r->n;
			r->n = 0;
			spinlock_unlock(&r->lock);
		
			/* append rq[i + 1] to rq[i] */
			r = &CPU->rq[i];
			spinlock_lock(&r->lock);
			list_concat(&r->rq_head, &head);
			r->n += n;
			spinlock_unlock(&r->lock);
		}
		CPU->needs_relink = 0;
	}
	spinlock_unlock(&CPU->lock);

}

/** The scheduler
 *
 * The thread scheduling procedure.
 * Passes control directly to
 * scheduler_separated_stack().
 *
 */
void scheduler(void)
{
	volatile ipl_t ipl;

	ASSERT(CPU != NULL);

	ipl = interrupts_disable();

	if (atomic_get(&haltstate))
		halt();
	
	if (THREAD) {
		spinlock_lock(&THREAD->lock);
		
		/* Update thread kernel accounting */
		THREAD->kcycles += get_cycle() - THREAD->last_cycle;
		
#ifndef CONFIG_FPU_LAZY
		fpu_context_save(THREAD->saved_fpu_context);
#endif
		if (!context_save(&THREAD->saved_context)) {
			/*
			 * This is the place where threads leave scheduler();
			 */
			
			/* Save current CPU cycle */
			THREAD->last_cycle = get_cycle();
			
			spinlock_unlock(&THREAD->lock);
			interrupts_restore(THREAD->saved_context.ipl);
			
			return;
		}

		/*
		 * Interrupt priority level of preempted thread is recorded
		 * here to facilitate scheduler() invocations from
		 * interrupts_disable()'d code (e.g. waitq_sleep_timeout()). 
		 */
		THREAD->saved_context.ipl = ipl;
	}

	/*
	 * Through the 'THE' structure, we keep track of THREAD, TASK, CPU, VM
	 * and preemption counter. At this point THE could be coming either
	 * from THREAD's or CPU's stack.
	 */
	the_copy(THE, (the_t *) CPU->stack);

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
	 */
	context_save(&CPU->saved_context);
	context_set(&CPU->saved_context, FADDR(scheduler_separated_stack),
	    (uintptr_t) CPU->stack, CPU_STACK_SIZE);
	context_restore(&CPU->saved_context);
	/* not reached */
}

/** Scheduler stack switch wrapper
 *
 * Second part of the scheduler() function
 * using new stack. Handling the actual context
 * switch to a new thread.
 *
 * Assume THREAD->lock is held.
 */
void scheduler_separated_stack(void)
{
	int priority;
	DEADLOCK_PROBE_INIT(p_joinwq);

	ASSERT(CPU != NULL);
	
	if (THREAD) {
		/* must be run after the switch to scheduler stack */
		after_thread_ran();

		switch (THREAD->state) {
		case Running:
			spinlock_unlock(&THREAD->lock);
			thread_ready(THREAD);
			break;

		case Exiting:
repeat:
			if (THREAD->detached) {
				thread_destroy(THREAD);
			} else {
				/*
				 * The thread structure is kept allocated until
				 * somebody calls thread_detach() on it.
				 */
				if (!spinlock_trylock(&THREAD->join_wq.lock)) {
					/*
					 * Avoid deadlock.
					 */
					spinlock_unlock(&THREAD->lock);
					delay(HZ);
					spinlock_lock(&THREAD->lock);
					DEADLOCK_PROBE(p_joinwq,
					    DEADLOCK_THRESHOLD);
					goto repeat;
				}
				_waitq_wakeup_unsafe(&THREAD->join_wq,
				    WAKEUP_FIRST);
				spinlock_unlock(&THREAD->join_wq.lock);
				
				THREAD->state = Lingering;
				spinlock_unlock(&THREAD->lock);
			}
			break;
			
		case Sleeping:
			/*
			 * Prefer the thread after it's woken up.
			 */
			THREAD->priority = -1;

			/*
			 * We need to release wq->lock which we locked in
			 * waitq_sleep(). Address of wq->lock is kept in
			 * THREAD->sleep_queue.
			 */
			spinlock_unlock(&THREAD->sleep_queue->lock);

			/*
			 * Check for possible requests for out-of-context
			 * invocation.
			 */
			if (THREAD->call_me) {
				THREAD->call_me(THREAD->call_me_with);
				THREAD->call_me = NULL;
				THREAD->call_me_with = NULL;
			}

			spinlock_unlock(&THREAD->lock);

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
	
	spinlock_lock(&THREAD->lock);
	priority = THREAD->priority;
	spinlock_unlock(&THREAD->lock);	

	relink_rq(priority);		

	/*
	 * If both the old and the new task are the same, lots of work is
	 * avoided.
	 */
	if (TASK != THREAD->task) {
		as_t *as1 = NULL;
		as_t *as2;

		if (TASK) {
			spinlock_lock(&TASK->lock);
			as1 = TASK->as;
			spinlock_unlock(&TASK->lock);
		}

		spinlock_lock(&THREAD->task->lock);
		as2 = THREAD->task->as;
		spinlock_unlock(&THREAD->task->lock);
		
		/*
		 * Note that it is possible for two tasks to share one address
		 * space.
		 */
		if (as1 != as2) {
			/*
			 * Both tasks and address spaces are different.
			 * Replace the old one with the new one.
			 */
			as_switch(as1, as2);
		}
		TASK = THREAD->task;
		before_task_runs();
	}

	spinlock_lock(&THREAD->lock);	
	THREAD->state = Running;

#ifdef SCHEDULER_VERBOSE
	printf("cpu%u: tid %" PRIu64 " (priority=%d, ticks=%" PRIu64 
	    ", nrdy=%ld)\n", CPU->id, THREAD->tid, THREAD->priority,
	    THREAD->ticks, atomic_get(&CPU->nrdy));
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
	the_copy(THE, (the_t *) THREAD->kstack);
	
	context_restore(&THREAD->saved_context);
	/* not reached */
}

#ifdef CONFIG_SMP
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
	thread_t *t;
	int count;
	atomic_count_t average;
	unsigned int i;
	int j;
	int k = 0;
	ipl_t ipl;

	/*
	 * Detach kcpulb as nobody will call thread_join_timeout() on it.
	 */
	thread_detach(THREAD);
	
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
	 */
	average = atomic_get(&nrdy) / config.cpu_active + 1;
	count = average - atomic_get(&CPU->nrdy);

	if (count <= 0)
		goto satisfied;

	/*
	 * Searching least priority queues on all CPU's first and most priority
	 * queues on all CPU's last.
	 */
	for (j = RQ_COUNT - 1; j >= 0; j--) {
		for (i = 0; i < config.cpu_active; i++) {
			link_t *l;
			runq_t *r;
			cpu_t *cpu;

			cpu = &cpus[(i + k) % config.cpu_active];

			/*
			 * Not interested in ourselves.
			 * Doesn't require interrupt disabling for kcpulb has
			 * THREAD_FLAG_WIRED.
			 */
			if (CPU == cpu)
				continue;
			if (atomic_get(&cpu->nrdy) <= average)
				continue;

			ipl = interrupts_disable();
			r = &cpu->rq[j];
			spinlock_lock(&r->lock);
			if (r->n == 0) {
				spinlock_unlock(&r->lock);
				interrupts_restore(ipl);
				continue;
			}
		
			t = NULL;
			l = r->rq_head.prev;	/* search rq from the back */
			while (l != &r->rq_head) {
				t = list_get_instance(l, thread_t, rq_link);
				/*
				 * We don't want to steal CPU-wired threads
				 * neither threads already stolen. The latter
				 * prevents threads from migrating between CPU's
				 * without ever being run. We don't want to
				 * steal threads whose FPU context is still in
				 * CPU.
				 */
				spinlock_lock(&t->lock);
				if ((!(t->flags & (THREAD_FLAG_WIRED |
				    THREAD_FLAG_STOLEN))) &&
				    (!(t->fpu_context_engaged))) {
					/*
					 * Remove t from r.
					 */
					spinlock_unlock(&t->lock);
					
					atomic_dec(&cpu->nrdy);
					atomic_dec(&nrdy);

					r->n--;
					list_remove(&t->rq_link);

					break;
				}
				spinlock_unlock(&t->lock);
				l = l->prev;
				t = NULL;
			}
			spinlock_unlock(&r->lock);

			if (t) {
				/*
				 * Ready t on local CPU
				 */
				spinlock_lock(&t->lock);
#ifdef KCPULB_VERBOSE
				printf("kcpulb%u: TID %" PRIu64 " -> cpu%u, "
				    "nrdy=%ld, avg=%ld\n", CPU->id, t->tid,
				    CPU->id, atomic_get(&CPU->nrdy),
				    atomic_get(&nrdy) / config.cpu_active);
#endif
				t->flags |= THREAD_FLAG_STOLEN;
				t->state = Entering;
				spinlock_unlock(&t->lock);
	
				thread_ready(t);

				interrupts_restore(ipl);
	
				if (--count == 0)
					goto satisfied;
					
				/*
				 * We are not satisfied yet, focus on another
				 * CPU next time.
				 */
				k++;
				
				continue;
			}
			interrupts_restore(ipl);
		}
	}

	if (atomic_get(&CPU->nrdy)) {
		/*
		 * Be a little bit light-weight and let migrated threads run.
		 */
		scheduler();
	} else {
		/*
		 * We failed to migrate a single thread.
		 * Give up this turn.
		 */
		goto loop;
	}
		
	goto not_satisfied;

satisfied:
	goto loop;
}

#endif /* CONFIG_SMP */


/** Print information about threads & scheduler queues */
void sched_print_list(void)
{
	ipl_t ipl;
	unsigned int cpu, i;
	runq_t *r;
	thread_t *t;
	link_t *cur;

	/* We are going to mess with scheduler structures,
	 * let's not be interrupted */
	ipl = interrupts_disable();
	for (cpu = 0; cpu < config.cpu_count; cpu++) {

		if (!cpus[cpu].active)
			continue;

		spinlock_lock(&cpus[cpu].lock);
		printf("cpu%u: address=%p, nrdy=%ld, needs_relink=%" PRIs "\n",
		    cpus[cpu].id, &cpus[cpu], atomic_get(&cpus[cpu].nrdy),
		    cpus[cpu].needs_relink);
		
		for (i = 0; i < RQ_COUNT; i++) {
			r = &cpus[cpu].rq[i];
			spinlock_lock(&r->lock);
			if (!r->n) {
				spinlock_unlock(&r->lock);
				continue;
			}
			printf("\trq[%u]: ", i);
			for (cur = r->rq_head.next; cur != &r->rq_head;
				cur = cur->next) {
				t = list_get_instance(cur, thread_t, rq_link);
				printf("%" PRIu64 "(%s) ", t->tid,
				    thread_states[t->state]);
			}
			printf("\n");
			spinlock_unlock(&r->lock);
		}
		spinlock_unlock(&cpus[cpu].lock);
	}
	
	interrupts_restore(ipl);
}

/** @}
 */
