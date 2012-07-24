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

#include <adt/list.h>
#include <synch/semaphore.h>
#include <compiler/barrier.h>


/* Fwd decl. */
struct thread;
struct rcu_item;

/** Grace period number typedef. */
typedef uint64_t rcu_gp_t;

/** RCU callback type. The passed rcu_item_t maybe freed. */
typedef void (*rcu_func_t)(struct rcu_item *rcu_item);

typedef struct rcu_item {
	rcu_func_t func;
	struct rcu_item *next;
} rcu_item_t;


/** RCU related per-cpu data. */
typedef struct rcu_cpu_data {
	/** The cpu recorded a quiescent state last time during this grace period */
	rcu_gp_t last_seen_gp;
	
	/** Pointer to the currently used nesting count (THREAD's or CPU's). */
	size_t *pnesting_cnt;
	/** Temporary nesting count if THREAD is NULL, eg in scheduler(). */
	size_t tmp_nesting_cnt;

	/** Callbacks to invoke once the current grace period ends, ie cur_cbs_gp.
	 * Accessed by the local reclaimer only.
	 */
	rcu_item_t *cur_cbs;
	/** Number of callbacks in cur_cbs. */
	size_t cur_cbs_cnt;
	/** Callbacks to invoke once the next grace period ends, ie next_cbs_gp. 
	 * Accessed by the local reclaimer only.
	 */
	rcu_item_t *next_cbs;
	/** Number of callbacks in next_cbs. */
	size_t next_cbs_cnt;
	/** New callbacks are place at the end of this list. */
	rcu_item_t *arriving_cbs;
	/** Tail of arriving_cbs list. Disable interrupts to access. */
	rcu_item_t **parriving_cbs_tail;
	/** Number of callbacks currently in arriving_cbs. 
	 * Disable interrupts to access.
	 */
	size_t arriving_cbs_cnt;

	/** At the end of this grace period callbacks in cur_cbs will be invoked.*/
	rcu_gp_t cur_cbs_gp;
	/** At the end of this grace period callbacks in next_cbs will be invoked.
	 * 
	 * Should be the next grace period but it allows the reclaimer to 
	 * notice if it missed a grace period end announcement. In that
	 * case it can execute next_cbs without waiting for another GP.
	 * 
	 * Invariant: next_cbs_gp >= cur_cbs_gp
	 */
	rcu_gp_t next_cbs_gp;
	
	/** This cpu has not yet passed a quiescent state and it is delaying the
	 * detector. Once it reaches a QS it must sema_up(rcu.remaining_readers).
	 */
	bool is_delaying_gp;
	
	/** Positive if there are callbacks pending in arriving_cbs. */
	semaphore_t arrived_flag;
	
	/** The reclaimer should expedite GPs for cbs in arriving_cbs. */
	bool expedite_arriving;
	
	/** Interruptable attached reclaimer thread. */
	struct thread *reclaimer_thr;
	
	/* Some statistics. */
	size_t stat_max_cbs;
	size_t stat_avg_cbs;
	size_t stat_missed_gps;
	size_t stat_missed_gp_in_wait;
	size_t stat_max_slice_cbs;
	size_t last_arriving_cnt;
} rcu_cpu_data_t;


/** RCU related per-thread data. */
typedef struct rcu_thread_data {
	/** The number of times an RCU reader section is nested. 
	 * 
	 * If positive, it is definitely executing reader code. If zero, 
	 * the thread might already be executing reader code thanks to
	 * cpu instruction reordering.
	 */
	size_t nesting_cnt;
	
	/** True if the thread was preempted in a reader section. 
	 *
	 * The thread is place into rcu.cur_preempted or rcu.next_preempted
	 * and must remove itself in rcu_read_unlock(). 
	 * 
	 * Access with interrupts disabled.
	 */
	bool was_preempted;
	/** Preempted threads link. Access with rcu.prempt_lock.*/
	link_t preempt_link;
} rcu_thread_data_t;


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

extern void rcu_read_lock(void);
extern void rcu_read_unlock(void);
extern bool rcu_read_locked(void);
extern void rcu_synchronize(void);
extern void rcu_call(rcu_item_t *rcu_item, rcu_func_t func);

extern void rcu_print_stat(void);

extern void rcu_init(void);
extern void rcu_stop(void);
extern void rcu_cpu_init(void);
extern void rcu_kinit_init(void);
extern void rcu_thread_init(struct thread*);
extern void rcu_thread_exiting(void);
extern void rcu_after_thread_ran(void);
extern void rcu_before_thread_runs(void);

/* Debugging/testing support. Not part of public API. Do not use! */
extern uint64_t rcu_completed_gps(void);
extern void _rcu_call(bool expedite, rcu_item_t *rcu_item, rcu_func_t func);

#endif

/** @}
 */
