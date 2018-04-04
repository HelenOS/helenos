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

/** @addtogroup sync
 * @{
 */
/** @file
 */

#ifndef KERN_RCU_H_
#define KERN_RCU_H_

#include <assert.h>
#include <synch/rcu_types.h>
#include <compiler/barrier.h>


/** Use to assign a pointer to newly initialized data to a rcu reader
 * accessible pointer.
 *
 * Example:
 * @code
 * typedef struct exam {
 *     struct exam *next;
 *     int grade;
 * } exam_t;
 *
 * exam_t *exam_list;
 * // ..
 *
 * // Insert at the beginning of the list.
 * exam_t *my_exam = malloc(sizeof(exam_t), 0);
 * my_exam->grade = 5;
 * my_exam->next = exam_list;
 * rcu_assign(exam_list, my_exam);
 *
 * // Changes properly propagate. Every reader either sees
 * // the old version of exam_list or the new version with
 * // the fully initialized my_exam.
 * rcu_synchronize();
 * // Now we can be sure every reader sees my_exam.
 *
 * @endcode
 */
#define rcu_assign(ptr, value) \
	do { \
		memory_barrier(); \
		(ptr) = (value); \
	} while (0)

/** Use to access RCU protected data in a reader section.
 *
 * Example:
 * @code
 * exam_t *exam_list;
 * // ...
 *
 * rcu_read_lock();
 * exam_t *first_exam = rcu_access(exam_list);
 * // We can now safely use first_exam, it won't change
 * // under us while we're using it.
 *
 * // ..
 * rcu_read_unlock();
 * @endcode
 */
#define rcu_access(ptr) ACCESS_ONCE(ptr)




#include <debug.h>
#include <preemption.h>
#include <cpu.h>
#include <proc/thread.h>


extern bool rcu_read_locked(void);
extern void rcu_synchronize(void);
extern void rcu_synchronize_expedite(void);
extern void rcu_call(rcu_item_t *rcu_item, rcu_func_t func);
extern void rcu_barrier(void);

extern void rcu_print_stat(void);

extern void rcu_init(void);
extern void rcu_stop(void);
extern void rcu_cpu_init(void);
extern void rcu_kinit_init(void);
extern void rcu_thread_init(struct thread *);
extern void rcu_thread_exiting(void);
extern void rcu_after_thread_ran(void);
extern void rcu_before_thread_runs(void);

extern uint64_t rcu_completed_gps(void);
extern void _rcu_call(bool expedite, rcu_item_t *rcu_item, rcu_func_t func);
extern void _rcu_synchronize(bool expedite);


#ifdef RCU_PREEMPT_A

#define RCU_CNT_INC       (1 << 1)
#define RCU_WAS_PREEMPTED (1 << 0)

/* Fwd. decl. because of inlining. */
void _rcu_preempted_unlock(void);

/** Delimits the start of an RCU reader critical section.
 *
 * Reader sections may be nested and are preemptible. You must not
 * however block/sleep within reader sections.
 */
static inline void rcu_read_lock(void)
{
	THE->rcu_nesting += RCU_CNT_INC;
	compiler_barrier();
}

/** Delimits the end of an RCU reader critical section. */
static inline void rcu_read_unlock(void)
{
	compiler_barrier();
	THE->rcu_nesting -= RCU_CNT_INC;

	if (RCU_WAS_PREEMPTED == THE->rcu_nesting) {
		_rcu_preempted_unlock();
	}
}

#elif defined(RCU_PREEMPT_PODZIMEK)

/* Fwd decl. required by the inlined implementation. Not part of public API. */
extern rcu_gp_t _rcu_cur_gp;
extern void _rcu_signal_read_unlock(void);


/** Unconditionally records a quiescent state for the local cpu. */
static inline void _rcu_record_qs(void)
{
	assert(PREEMPTION_DISABLED || interrupts_disabled());

	/*
	 * A new GP was started since the last time we passed a QS.
	 * Notify the detector we have reached a new QS.
	 */
	if (CPU->rcu.last_seen_gp != _rcu_cur_gp) {
		rcu_gp_t cur_gp = ACCESS_ONCE(_rcu_cur_gp);
		/*
		 * Contain memory accesses within a reader critical section.
		 * If we are in rcu_lock() it also makes changes prior to the
		 * start of the GP visible in the reader section.
		 */
		memory_barrier();
		/*
		 * Acknowledge we passed a QS since the beginning of rcu.cur_gp.
		 * Cache coherency will lazily transport the value to the
		 * detector while it sleeps in gp_sleep().
		 *
		 * Note that there is a theoretical possibility that we
		 * overwrite a more recent/greater last_seen_gp here with
		 * an older/smaller value. If this cpu is interrupted here
		 * while in rcu_lock() reader sections in the interrupt handler
		 * will update last_seen_gp to the same value as is currently
		 * in local cur_gp. However, if the cpu continues processing
		 * interrupts and the detector starts a new GP immediately,
		 * local interrupt handlers may update last_seen_gp again (ie
		 * properly ack the new GP) with a value greater than local cur_gp.
		 * Resetting last_seen_gp to a previous value here is however
		 * benign and we only have to remember that this reader may end up
		 * in cur_preempted even after the GP ends. That is why we
		 * append next_preempted to cur_preempted rather than overwriting
		 * it as if cur_preempted were empty.
		 */
		CPU->rcu.last_seen_gp = cur_gp;
	}
}

/** Delimits the start of an RCU reader critical section.
 *
 * Reader sections may be nested and are preemptable. You must not
 * however block/sleep within reader sections.
 */
static inline void rcu_read_lock(void)
{
	assert(CPU);
	preemption_disable();

	/* Record a QS if not in a reader critical section. */
	if (0 == CPU->rcu.nesting_cnt)
		_rcu_record_qs();

	++CPU->rcu.nesting_cnt;

	preemption_enable();
}

/** Delimits the end of an RCU reader critical section. */
static inline void rcu_read_unlock(void)
{
	assert(CPU);
	preemption_disable();

	if (0 == --CPU->rcu.nesting_cnt) {
		_rcu_record_qs();

		/*
		 * The thread was preempted while in a critical section or
		 * the detector is eagerly waiting for this cpu's reader to finish.
		 */
		if (CPU->rcu.signal_unlock) {
			/* Rechecks with disabled interrupts. */
			_rcu_signal_read_unlock();
		}
	}

	preemption_enable();
}
#endif

#endif

/** @}
 */
