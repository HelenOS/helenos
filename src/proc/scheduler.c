/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <cpu.h>
#include <mm/vm.h>
#include <config.h>
#include <context.h>
#include <func.h>
#include <arch.h>
#include <arch/asm.h>
#include <list.h>
#include <typedefs.h>
#include <mm/page.h>
#include <synch/spinlock.h>

#ifdef __SMP__
#include <arch/smp/atomic.h>
#endif /* __SMP__ */

/*
 * NOTE ON ATOMIC READS:
 * Some architectures cannot read __u32 atomically.
 * For that reason, all accesses to nrdy and the likes must be protected by spinlock.
 */

spinlock_t nrdylock;
volatile int nrdy;

void scheduler_init(void)
{
	spinlock_initialize(&nrdylock);
}

/* cpu_priority_high()'d */
struct thread *find_best_thread(void)
{
	thread_t *t;
	runq_t *r;
	int i, n;

loop:
	cpu_priority_high();

	spinlock_lock(&the->cpu->lock);
	n = the->cpu->nrdy;
	spinlock_unlock(&the->cpu->lock);

	cpu_priority_low();
	
	if (n == 0) {
		#ifdef __SMP__
		/*
		 * If the load balancing thread is not running, wake it up and
		 * set CPU-private flag that the kcpulb has been started.
		 */
		if (test_and_set(&the->cpu->kcpulbstarted) == 0) {
    			waitq_wakeup(&the->cpu->kcpulb_wq, 0);
			goto loop;
		}
		#endif /* __SMP__ */
		
		/*
		 * For there was nothing to run, the CPU goes to sleep
		 * until a hardware interrupt or an IPI comes.
		 * This improves energy saving and hyperthreading.
		 * On the other hand, several hardware interrupts can be ignored.
		 */
		 cpu_sleep();
		 goto loop;
	}

	cpu_priority_high();

	for (i = 0; i<RQ_COUNT; i++) {
		r = &the->cpu->rq[i];
		spinlock_lock(&r->lock);
		if (r->n == 0) {
			/*
			 * If this queue is empty, try a lower-priority queue.
			 */
			spinlock_unlock(&r->lock);
			continue;
		}
	
		spinlock_lock(&nrdylock);
		nrdy--;
		spinlock_unlock(&nrdylock);		

		spinlock_lock(&the->cpu->lock);
		the->cpu->nrdy--;
		spinlock_unlock(&the->cpu->lock);

		r->n--;

		/*
		 * Take the first thread from the queue.
		 */
		t = list_get_instance(r->rq_head.next, thread_t, rq_link);
		list_remove(&t->rq_link);

		spinlock_unlock(&r->lock);

		spinlock_lock(&t->lock);
		t->cpu = the->cpu;

		t->ticks = us2ticks((i+1)*10000);
		t->pri = i;	/* eventually correct rq index */

		/*
		 * Clear the X_STOLEN flag so that t can be migrated when load balancing needs emerge.
		 */
		t->flags &= ~X_STOLEN;
		spinlock_unlock(&t->lock);

		return t;
	}
	goto loop;

}

/*
 * This function prevents low priority threads from starving in rq's.
 * When it decides to relink rq's, it reconnects respective pointers
 * so that in result threads with 'pri' greater or equal 'start' are
 * moved to a higher-priority queue.
 */
void relink_rq(int start)
{
	link_t head;
	runq_t *r;
	int i, n;

	list_initialize(&head);
	spinlock_lock(&the->cpu->lock);
	if (the->cpu->needs_relink > NEEDS_RELINK_MAX) {
		for (i = start; i<RQ_COUNT-1; i++) {
			/* remember and empty rq[i + 1] */
			r = &the->cpu->rq[i + 1];
			spinlock_lock(&r->lock);
			list_concat(&head, &r->rq_head);
			n = r->n;
			r->n = 0;
			spinlock_unlock(&r->lock);
		
			/* append rq[i + 1] to rq[i] */
			r = &the->cpu->rq[i];
			spinlock_lock(&r->lock);
			list_concat(&r->rq_head, &head);
			r->n += n;
			spinlock_unlock(&r->lock);
		}
		the->cpu->needs_relink = 0;
	}
	spinlock_unlock(&the->cpu->lock);				

}

/*
 * The scheduler.
 */
void scheduler(void)
{
	volatile pri_t pri;

	pri = cpu_priority_high();

	if (haltstate)
		halt();

	if (the->thread) {
		spinlock_lock(&the->thread->lock);
		if (!context_save(&the->thread->saved_context)) {
			/*
			 * This is the place where threads leave scheduler();
			 */
		    	spinlock_unlock(&the->thread->lock);
			cpu_priority_restore(the->thread->saved_context.pri);
			return;
		}
		the->thread->saved_context.pri = pri;
	}

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
	context_save(&the->cpu->saved_context);
	the->cpu->saved_context.sp = (__address) &the->cpu->stack[CPU_STACK_SIZE-8];
	the->cpu->saved_context.pc = (__address) scheduler_separated_stack;
	context_restore(&the->cpu->saved_context);
	/* not reached */
}

void scheduler_separated_stack(void)
{
	int priority;

	if (the->thread) {
		switch (the->thread->state) {
		    case Running:
			    the->thread->state = Ready;
			    spinlock_unlock(&the->thread->lock);
			    thread_ready(the->thread);
			    break;

		    case Exiting:
			    frame_free((__address) the->thread->kstack);
			    if (the->thread->ustack) {
				    frame_free((__address) the->thread->ustack);
			    }
			    
			    /*
			     * Detach from the containing task.
			     */
			    spinlock_lock(&the->task->lock);
			    list_remove(&the->thread->th_link);
			    spinlock_unlock(&the->task->lock);

			    spinlock_unlock(&the->thread->lock);
			    
			    spinlock_lock(&threads_lock);
			    list_remove(&the->thread->threads_link);
			    spinlock_unlock(&threads_lock);
			    
			    free(the->thread);
			    
			    break;
			    
		    case Sleeping:
			    /*
			     * Prefer the thread after it's woken up.
			     */
			    the->thread->pri = -1;

			    /*
			     * We need to release wq->lock which we locked in waitq_sleep().
			     * Address of wq->lock is kept in the->thread->sleep_queue.
			     */
			    spinlock_unlock(&the->thread->sleep_queue->lock);

			    /*
			     * Check for possible requests for out-of-context invocation.
			     */
			    if (the->thread->call_me) {
				    the->thread->call_me(the->thread->call_me_with);
				    the->thread->call_me = NULL;
				    the->thread->call_me_with = NULL;
			    }

			    spinlock_unlock(&the->thread->lock);
			    
			    break;

		    default:
			    /*
			     * Entering state is unexpected.
			     */
			    panic("tid%d: unexpected state %s\n", the->thread->tid, thread_states[the->thread->state]);
			    break;
		}
		the->thread = NULL;
	}
    
	the->thread = find_best_thread();
	
	spinlock_lock(&the->thread->lock);
	priority = the->thread->pri;
	spinlock_unlock(&the->thread->lock);	
	
	relink_rq(priority);		

	spinlock_lock(&the->thread->lock);	

	/*
	 * If both the old and the new task are the same, lots of work is avoided.
	 */
	if (the->task != the->thread->task) {
		vm_t *m1 = NULL;
		vm_t *m2;

		if (the->task) {
			spinlock_lock(&the->task->lock);
			m1 = the->task->vm;
			spinlock_unlock(&the->task->lock);
		}

		spinlock_lock(&the->thread->task->lock);
		m2 = the->thread->task->vm;
		spinlock_unlock(&the->thread->task->lock);
		
		/*
		 * Note that it is possible for two tasks to share one vm mapping.
		 */
		if (m1 != m2) {
			/*
			 * Both tasks and vm mappings are different.
			 * Replace the old one with the new one.
			 */
			if (m1) {
				vm_uninstall(m1);
			}
			vm_install(m2);
		}
		the->task = the->thread->task;	
	}

	the->thread->state = Running;

	#ifdef SCHEDULER_VERBOSE
	printf("cpu%d: tid %d (pri=%d,ticks=%d,nrdy=%d)\n", the->cpu->id, the->thread->tid, the->thread->pri, the->thread->ticks, the->cpu->nrdy);
	#endif	

	context_restore(&the->thread->saved_context);
	/* not reached */
}

#ifdef __SMP__
/*
 * This is the load balancing thread. 
 * It supervises thread supplies for the CPU it's wired to.
 */
void kcpulb(void *arg)
{
	thread_t *t;
	int count, i, j, k = 0;
	pri_t pri;

loop:
	/*
	 * Sleep until there's some work to do.
	 */
	waitq_sleep(&the->cpu->kcpulb_wq);

not_satisfied:
	/*
	 * Calculate the number of threads that will be migrated/stolen from
	 * other CPU's. Note that situation can have changed between two
	 * passes. Each time get the most up to date counts.
	 */
	pri = cpu_priority_high();
	spinlock_lock(&the->cpu->lock);
	count = nrdy / config.cpu_active;
	count -= the->cpu->nrdy;
	spinlock_unlock(&the->cpu->lock);
	cpu_priority_restore(pri);

	if (count <= 0)
		goto satisfied;

	/*
	 * Searching least priority queues on all CPU's first and most priority queues on all CPU's last.
	 */
	for (j=RQ_COUNT-1; j >= 0; j--) {
		for (i=0; i < config.cpu_active; i++) {
			link_t *l;
			runq_t *r;
			cpu_t *cpu;

			cpu = &cpus[(i + k) % config.cpu_active];
			r = &cpu->rq[j];

			/*
			 * Not interested in ourselves.
			 * Doesn't require interrupt disabling for kcpulb is X_WIRED.
			 */
			if (the->cpu == cpu)
				continue;

restart:		pri = cpu_priority_high();
			spinlock_lock(&r->lock);
			if (r->n == 0) {
				spinlock_unlock(&r->lock);
				cpu_priority_restore(pri);
				continue;
			}
		
			t = NULL;
			l = r->rq_head.prev;	/* search rq from the back */
			while (l != &r->rq_head) {
				t = list_get_instance(l, thread_t, rq_link);
				/*
		    		 * We don't want to steal CPU-wired threads neither threads already stolen.
				 * The latter prevents threads from migrating between CPU's without ever being run.
		        	 */
				spinlock_lock(&t->lock);
				if (!(t->flags & (X_WIRED | X_STOLEN))) {
					/*
					 * Remove t from r.
					 */

					spinlock_unlock(&t->lock);
					
					/*
					 * Here we have to avoid deadlock with relink_rq(),
					 * because it locks cpu and r in a different order than we do.
					 */
					if (!spinlock_trylock(&cpu->lock)) {
						/* Release all locks and try again. */ 
						spinlock_unlock(&r->lock);
						cpu_priority_restore(pri);
						goto restart;
					}
					cpu->nrdy--;
					spinlock_unlock(&cpu->lock);

					spinlock_lock(&nrdylock);
					nrdy--;
					spinlock_unlock(&nrdylock);					

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
				printf("kcpulb%d: TID %d -> cpu%d, nrdy=%d, avg=%d\n", the->cpu->id, t->tid, the->cpu->id, the->cpu->nrdy, nrdy / config.cpu_active);
				#endif
				t->flags |= X_STOLEN;
				spinlock_unlock(&t->lock);
	
				thread_ready(t);

				cpu_priority_restore(pri);
	
				if (--count == 0)
					goto satisfied;
					
				/*
                    		 * We are not satisfied yet, focus on another CPU next time.
				 */
				k++;
				
				continue;
			}
			cpu_priority_restore(pri);
		}
	}

	if (the->cpu->nrdy) {
		/*
		 * Be a little bit light-weight and let migrated threads run.
		 */
		scheduler();
	} 
	else {
		/*
		 * We failed to migrate a single thread.
		 * Something more sophisticated should be done.
		 */
		scheduler();
	}
		
	goto not_satisfied;
    
satisfied:
	/*
	 * Tell find_best_thread() to wake us up later again.
	 */
	the->cpu->kcpulbstarted = 0;
	goto loop;
}

#endif /* __SMP__ */
