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

/**
 * @file
 * @brief Preemptible read-copy update. Usable from interrupt handlers.
 */
 
#include <synch/rcu.h>
#include <synch/condvar.h>
#include <synch/semaphore.h>
#include <synch/spinlock.h>
#include <proc/thread.h>
#include <cpu/cpu_mask.h>
#include <cpu.h>
#include <smp/smp_call.h>
#include <compiler/barrier.h>
#include <atomic.h>
#include <arch.h>
#include <macros.h>

/* 
 * Number of milliseconds to give to preexisting readers to finish 
 * when non-expedited grace period detection is in progress.
 */
#define DETECT_SLEEP_MS    10
/* 
 * Max number of pending callbacks in the local cpu's queue before 
 * aggressively expediting the current grace period
 */
#define EXPEDITE_THRESHOLD 2000
/*
 * Max number of callbacks to execute in one go with preemption
 * enabled. If there are more callbacks to be executed they will
 * be run with preemption disabled in order to prolong reclaimer's
 * time slice and give it a chance to catch up with callback producers.
 */
#define CRITICAL_THRESHOLD 30000
/* Half the number of values a uint32 can hold. */
#define UINT32_MAX_HALF    2147483648U


/** Global RCU data. */
typedef struct rcu_data {
	/** Detector uses so signal reclaimers that a grace period ended. */
	condvar_t gp_ended;
	/** Reclaimers notify the detector when they request more grace periods.*/
	condvar_t req_gp_changed;
	/** Reclaimers use to notify the detector to accelerate GP detection. */
	condvar_t expedite_now;
	/** 
	 * The detector waits on this semaphore for any readers delaying the GP.
	 * 
	 * Each of the cpus with readers that are delaying the current GP 
	 * must up() this sema once they reach a quiescent state. If there 
	 * are any readers in cur_preempted (ie preempted preexisting) and 
	 * they are already delaying GP detection, the last to unlock its
	 * reader section must up() this sema once.
	 */
	semaphore_t remaining_readers;
	
	/** Protects the 4 fields below. */
	SPINLOCK_DECLARE(gp_lock);
	/** Number of grace period ends the detector was requested to announce. */
	size_t req_gp_end_cnt;
	/** Number of consecutive grace periods to detect quickly and aggressively.*/
	size_t req_expedited_cnt;
	/** 
	 * The current grace period number. Increases monotonically. 
	 * Lock gp_lock or preempt_lock to get a current value.
	 */
	rcu_gp_t cur_gp;
	/**
	 * The number of the most recently completed grace period. 
	 * At most one behind cur_gp. If equal to cur_gp, a grace 
	 * period detection is not in progress and the detector
	 * is idle.
	 */
	rcu_gp_t completed_gp;
	
	/** Protects the following 3 fields. */
	IRQ_SPINLOCK_DECLARE(preempt_lock);
	/** Preexisting readers that have been preempted. */
	list_t cur_preempted;
	/** Reader that have been preempted and might delay the next grace period.*/
	list_t next_preempted;
	/** 
	 * The detector is waiting for the last preempted reader 
	 * in cur_preempted to announce that it exited its reader 
	 * section by up()ing remaining_readers.
	 */
	bool preempt_blocking_det;
	
	/** 
	 * Number of cpus with readers that are delaying the current GP.
	 * They will up() remaining_readers.
	 */
	atomic_t delaying_cpu_cnt;
	
	/** Interruptible attached detector thread pointer. */
	thread_t *detector_thr;
	
	/* Some statistics. */
	size_t stat_expedited_cnt;
	size_t stat_delayed_cnt;
	size_t stat_preempt_blocking_cnt;
	/* Does not contain self/local calls. */
	size_t stat_smp_call_cnt;
} rcu_data_t;


static rcu_data_t rcu;

static void start_detector(void);
static void start_reclaimers(void);
static void rcu_read_unlock_impl(size_t *pnesting_cnt);
static void synch_complete(rcu_item_t *rcu_item);
static void check_qs(void);
static void record_qs(void);
static void signal_read_unlock(void);
static bool arriving_cbs_empty(void);
static bool next_cbs_empty(void);
static bool cur_cbs_empty(void);
static bool all_cbs_empty(void);
static void reclaimer(void *arg);
static bool wait_for_pending_cbs(void);
static bool advance_cbs(void);
static void exec_completed_cbs(rcu_gp_t last_completed_gp);
static void exec_cbs(rcu_item_t **phead);
static void req_detection(size_t req_cnt);
static bool wait_for_cur_cbs_gp_end(bool expedite, rcu_gp_t *last_completed_gp);
static void upd_missed_gp_in_wait(rcu_gp_t completed_gp);
static bool cv_wait_for_gp(rcu_gp_t wait_on_gp);
static void detector(void *);
static bool wait_for_detect_req(void);
static void start_new_gp(void);
static void end_cur_gp(void);
static bool wait_for_readers(void);
static void rm_quiescent_cpus(cpu_mask_t *cpu_mask);
static bool gp_sleep(void);
static void interrupt_delaying_cpus(cpu_mask_t *cpu_mask);
static void sample_local_cpu(void *);
static bool wait_for_delaying_cpus(void);
static bool wait_for_preempt_reader(void);
static void upd_max_cbs_in_slice(void);



/** Initializes global RCU structures. */
void rcu_init(void)
{
	condvar_initialize(&rcu.gp_ended);
	condvar_initialize(&rcu.req_gp_changed);
	condvar_initialize(&rcu.expedite_now);
	semaphore_initialize(&rcu.remaining_readers, 0);
	
	spinlock_initialize(&rcu.gp_lock, "rcu.gp_lock");
	rcu.req_gp_end_cnt = 0;
	rcu.req_expedited_cnt = 0;
	rcu.cur_gp = 0;
	rcu.completed_gp = 0;
	
	irq_spinlock_initialize(&rcu.preempt_lock, "rcu.preempt_lock");
	list_initialize(&rcu.cur_preempted);
	list_initialize(&rcu.next_preempted);
	rcu.preempt_blocking_det = false;
	
	atomic_set(&rcu.delaying_cpu_cnt, 0);
	
	rcu.detector_thr = 0;
	
	rcu.stat_expedited_cnt = 0;
	rcu.stat_delayed_cnt = 0;
	rcu.stat_preempt_blocking_cnt = 0;
	rcu.stat_smp_call_cnt = 0;
}

/** Initializes per-CPU RCU data. If on the boot cpu inits global data too.*/
void rcu_cpu_init(void)
{
	if (config.cpu_active == 1) {
		rcu_init();
	}
	
	CPU->rcu.last_seen_gp = 0;
	
	CPU->rcu.pnesting_cnt = &CPU->rcu.tmp_nesting_cnt;
	CPU->rcu.tmp_nesting_cnt = 0;
	
	CPU->rcu.cur_cbs = 0;
	CPU->rcu.cur_cbs_cnt = 0;
	CPU->rcu.next_cbs = 0;
	CPU->rcu.next_cbs_cnt = 0;
	CPU->rcu.arriving_cbs = 0;
	CPU->rcu.parriving_cbs_tail = &CPU->rcu.arriving_cbs;
	CPU->rcu.arriving_cbs_cnt = 0;

	CPU->rcu.cur_cbs_gp = 0;
	CPU->rcu.next_cbs_gp = 0;
	
	CPU->rcu.is_delaying_gp = false;
	
	semaphore_initialize(&CPU->rcu.arrived_flag, 0);

	/* BSP creates reclaimer threads before AP's rcu_cpu_init() runs. */
	if (config.cpu_active == 1)
		CPU->rcu.reclaimer_thr = 0;
	
	CPU->rcu.stat_max_cbs = 0;
	CPU->rcu.stat_avg_cbs = 0;
	CPU->rcu.stat_missed_gps = 0;
	CPU->rcu.stat_missed_gp_in_wait = 0;
	CPU->rcu.stat_max_slice_cbs = 0;
	CPU->rcu.last_arriving_cnt = 0;
}

/** Completes RCU init. Creates and runs the detector and reclaimer threads.*/
void rcu_kinit_init(void)
{
	start_detector();
	start_reclaimers();
}

/** Initializes any per-thread RCU structures. */
void rcu_thread_init(thread_t *thread)
{
	thread->rcu.nesting_cnt = 0;
	thread->rcu.was_preempted = false;
	link_initialize(&thread->rcu.preempt_link);
}

/** Called from scheduler() when exiting the current thread. 
 * 
 * Preemption or interrupts are disabled and the scheduler() already
 * switched away from the current thread, calling rcu_after_thread_ran().
 */
void rcu_thread_exiting(void)
{
	ASSERT(THREAD != 0);
	ASSERT(THREAD->state == Exiting);
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	/* 
	 * The scheduler() must have already switched to a temporary
	 * nesting counter for interrupt handlers (we could be idle)
	 * so that interrupt handlers do not modify the exiting thread's
	 * reader section nesting count while we examine/process it.
	 */
	ASSERT(&CPU->rcu.tmp_nesting_cnt == CPU->rcu.pnesting_cnt);
	
	/* 
	 * The thread forgot to exit its reader critical secion. 
	 * It is a bug, but rather than letting the entire system lock up
	 * forcefully leave the reader section. The thread is not holding 
	 * any references anyway since it is exiting so it is safe.
	 */
	if (0 < THREAD->rcu.nesting_cnt) {
		THREAD->rcu.nesting_cnt = 1;
		rcu_read_unlock_impl(&THREAD->rcu.nesting_cnt);
	}
}

/** Cleans up global RCU resources and stops dispatching callbacks. 
 * 
 * Call when shutting down the kernel. Outstanding callbacks will
 * not be processed. Instead they will linger forever.
 */
void rcu_stop(void)
{
	/* todo: stop accepting new callbacks instead of just letting them linger?*/
	
	/* Stop and wait for reclaimers. */
	for (unsigned int cpu_id = 0; cpu_id < config.cpu_active; ++cpu_id) {
		ASSERT(cpus[cpu_id].rcu.reclaimer_thr != 0);
	
		if (cpus[cpu_id].rcu.reclaimer_thr) {
			thread_interrupt(cpus[cpu_id].rcu.reclaimer_thr);
			thread_join(cpus[cpu_id].rcu.reclaimer_thr);
			thread_detach(cpus[cpu_id].rcu.reclaimer_thr);
			cpus[cpu_id].rcu.reclaimer_thr = 0;
		}
	}

	/* Stop the detector and wait. */
	if (rcu.detector_thr) {
		thread_interrupt(rcu.detector_thr);
		thread_join(rcu.detector_thr);
		thread_detach(rcu.detector_thr);
		rcu.detector_thr = 0;
	}
}

/** Starts the detector thread. */
static void start_detector(void)
{
	rcu.detector_thr = 
		thread_create(detector, 0, TASK, THREAD_FLAG_NONE, "rcu-det");
	
	if (!rcu.detector_thr) 
		panic("Failed to create RCU detector thread.");
	
	thread_ready(rcu.detector_thr);
}

/** Creates and runs cpu-bound reclaimer threads. */
static void start_reclaimers(void)
{
	for (unsigned int cpu_id = 0; cpu_id < config.cpu_count; ++cpu_id) {
		char name[THREAD_NAME_BUFLEN] = {0};
		
		snprintf(name, THREAD_NAME_BUFLEN - 1, "rcu-rec/%u", cpu_id);
		
		cpus[cpu_id].rcu.reclaimer_thr = 
			thread_create(reclaimer, 0, TASK, THREAD_FLAG_NONE, name);

		if (!cpus[cpu_id].rcu.reclaimer_thr) 
			panic("Failed to create RCU reclaimer thread on cpu%u.", cpu_id);

		thread_wire(cpus[cpu_id].rcu.reclaimer_thr, &cpus[cpu_id]);
		thread_ready(cpus[cpu_id].rcu.reclaimer_thr);
	}
}

/** Returns the number of elapsed grace periods since boot. */
uint64_t rcu_completed_gps(void)
{
	spinlock_lock(&rcu.gp_lock);
	uint64_t completed = rcu.completed_gp;
	spinlock_unlock(&rcu.gp_lock);
	
	return completed;
}

/** Delimits the start of an RCU reader critical section. 
 * 
 * Reader sections may be nested and are preemptable. You must not
 * however block/sleep within reader sections.
 */
void rcu_read_lock(void)
{
	ASSERT(CPU);
	preemption_disable();

	check_qs();
	++(*CPU->rcu.pnesting_cnt);

	preemption_enable();
}

/** Delimits the end of an RCU reader critical section. */
void rcu_read_unlock(void)
{
	ASSERT(CPU);
	preemption_disable();
	
	rcu_read_unlock_impl(CPU->rcu.pnesting_cnt);
	
	preemption_enable();
}

/** Unlocks the local reader section using the given nesting count. 
 * 
 * Preemption or interrupts must be disabled. 
 * 
 * @param pnesting_cnt Either &CPU->rcu.tmp_nesting_cnt or 
 *           THREAD->rcu.nesting_cnt.
 */
static void rcu_read_unlock_impl(size_t *pnesting_cnt)
{
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	
	if (0 == --(*pnesting_cnt)) {
		record_qs();
		
		/* 
		 * The thread was preempted while in a critical section or 
		 * the detector is eagerly waiting for this cpu's reader 
		 * to finish. 
		 * 
		 * Note that THREAD may be 0 in scheduler() and not just during boot.
		 */
		if ((THREAD && THREAD->rcu.was_preempted) || CPU->rcu.is_delaying_gp) {
			/* Rechecks with disabled interrupts. */
			signal_read_unlock();
		}
	}
}

/** Records a QS if not in a reader critical section. */
static void check_qs(void)
{
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	
	if (0 == *CPU->rcu.pnesting_cnt)
		record_qs();
}

/** Unconditionally records a quiescent state for the local cpu. */
static void record_qs(void)
{
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	
	/* 
	 * A new GP was started since the last time we passed a QS. 
	 * Notify the detector we have reached a new QS.
	 */
	if (CPU->rcu.last_seen_gp != rcu.cur_gp) {
		rcu_gp_t cur_gp = ACCESS_ONCE(rcu.cur_gp);
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

/** If necessary, signals the detector that we exited a reader section. */
static void signal_read_unlock(void)
{
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	
	/*
	 * We have to disable interrupts in order to make checking
	 * and resetting was_preempted and is_delaying_gp atomic 
	 * with respect to local interrupt handlers. Otherwise
	 * an interrupt could beat us to calling semaphore_up()
	 * before we reset the appropriate flag.
	 */
	ipl_t ipl = interrupts_disable();
	
	/* 
	 * If the detector is eagerly waiting for this cpu's reader to unlock,
	 * notify it that the reader did so.
	 */
	if (CPU->rcu.is_delaying_gp) {
		CPU->rcu.is_delaying_gp = false;
		semaphore_up(&rcu.remaining_readers);
	}
	
	/*
	 * This reader was preempted while in a reader section.
	 * We might be holding up the current GP. Notify the
	 * detector if so.
	 */
	if (THREAD && THREAD->rcu.was_preempted) {
		ASSERT(link_used(&THREAD->rcu.preempt_link));
		THREAD->rcu.was_preempted = false;

		irq_spinlock_lock(&rcu.preempt_lock, false);
		
		bool prev_empty = list_empty(&rcu.cur_preempted);
		list_remove(&THREAD->rcu.preempt_link);
		bool now_empty = list_empty(&rcu.cur_preempted);
		
		/* This was the last reader in cur_preempted. */
		bool last_removed = now_empty && !prev_empty;
		
		/* 
		 * Preempted readers are blocking the detector and 
		 * this was the last reader blocking the current GP. 
		 */
		if (last_removed && rcu.preempt_blocking_det) {
			rcu.preempt_blocking_det = false;
			semaphore_up(&rcu.remaining_readers);
		}
		
		irq_spinlock_unlock(&rcu.preempt_lock, false);
	}
	interrupts_restore(ipl);
}

typedef struct synch_item {
	waitq_t wq;
	rcu_item_t rcu_item;
} synch_item_t;

/** Blocks until all preexisting readers exit their critical sections. */
void rcu_synchronize(void)
{
	/* Calling from a reader section will deadlock. */
	ASSERT(THREAD == 0 || 0 == THREAD->rcu.nesting_cnt);
	
	synch_item_t completion; 

	waitq_initialize(&completion.wq);
	rcu_call(&completion.rcu_item, synch_complete);
	waitq_sleep(&completion.wq);
	waitq_complete_wakeup(&completion.wq);
}

/** rcu_synchronize's callback. */
static void synch_complete(rcu_item_t *rcu_item)
{
	synch_item_t *completion = member_to_inst(rcu_item, synch_item_t, rcu_item);
	ASSERT(completion);
	waitq_wakeup(&completion->wq, WAKEUP_FIRST);
}

/** Adds a callback to invoke after all preexisting readers finish. 
 * 
 * May be called from within interrupt handlers or RCU reader sections.
 * 
 * @param rcu_item Used by RCU to track the call. Must remain
 *         until the user callback function is entered.
 * @param func User callback function that will be invoked once a full
 *         grace period elapsed, ie at a time when all preexisting
 *         readers have finished. The callback should be short and must
 *         not block. If you must sleep, enqueue your work in the system
 *         work queue from the callback (ie workq_global_enqueue()).
 */
void rcu_call(rcu_item_t *rcu_item, rcu_func_t func)
{
	_rcu_call(false, rcu_item, func);
}

/** rcu_call() implementation. See rcu_call() for comments. */
void _rcu_call(bool expedite, rcu_item_t *rcu_item, rcu_func_t func)
{
	ASSERT(rcu_item);
	
	rcu_item->func = func;
	rcu_item->next = 0;
	
	preemption_disable();
	
	ipl_t ipl = interrupts_disable();

	*CPU->rcu.parriving_cbs_tail = rcu_item;
	CPU->rcu.parriving_cbs_tail = &rcu_item->next;
	
	size_t cnt = ++CPU->rcu.arriving_cbs_cnt;
	interrupts_restore(ipl);
	
	if (expedite) {
		CPU->rcu.expedite_arriving = true;
	}
	
	/* Added first callback - notify the reclaimer. */
	if (cnt == 1 && !semaphore_count_get(&CPU->rcu.arrived_flag)) {
		semaphore_up(&CPU->rcu.arrived_flag);
	}
	
	preemption_enable();
}

static bool cur_cbs_empty(void)
{
	ASSERT(THREAD && THREAD->wired);
	return 0 == CPU->rcu.cur_cbs;
}

static bool next_cbs_empty(void)
{
	ASSERT(THREAD && THREAD->wired);
	return 0 == CPU->rcu.next_cbs;
}

/** Disable interrupts to get an up-to-date result. */
static bool arriving_cbs_empty(void)
{
	ASSERT(THREAD && THREAD->wired);
	/* 
	 * Accessing with interrupts enabled may at worst lead to 
	 * a false negative if we race with a local interrupt handler.
	 */
	return 0 == CPU->rcu.arriving_cbs;
}

static bool all_cbs_empty(void)
{
	return cur_cbs_empty() && next_cbs_empty() && arriving_cbs_empty();
}

/** Reclaimer thread dispatches locally queued callbacks once a GP ends. */
static void reclaimer(void *arg)
{
	ASSERT(THREAD && THREAD->wired);
	ASSERT(THREAD == CPU->rcu.reclaimer_thr);

	rcu_gp_t last_compl_gp = 0;
	bool ok = true;
	
	while (ok && wait_for_pending_cbs()) {
		ASSERT(CPU->rcu.reclaimer_thr == THREAD);
		
		exec_completed_cbs(last_compl_gp);

		bool expedite = advance_cbs();
		
		ok = wait_for_cur_cbs_gp_end(expedite, &last_compl_gp);
	}
}

/** Waits until there are callbacks waiting to be dispatched. */
static bool wait_for_pending_cbs(void)
{
	if (!all_cbs_empty()) 
		return true;

	bool ok = true;
	
	while (arriving_cbs_empty() && ok) {
		ok = semaphore_down_interruptable(&CPU->rcu.arrived_flag);
	}
	
	return ok;
}

static void upd_stat_missed_gp(rcu_gp_t compl)
{
	if (CPU->rcu.cur_cbs_gp < compl) {
		CPU->rcu.stat_missed_gps += (size_t)(compl - CPU->rcu.cur_cbs_gp);
	}
}

/** Executes all callbacks for the given completed grace period. */
static void exec_completed_cbs(rcu_gp_t last_completed_gp)
{
	upd_stat_missed_gp(last_completed_gp);
	
	/* Both next_cbs and cur_cbs GP elapsed. */
	if (CPU->rcu.next_cbs_gp <= last_completed_gp) {
		ASSERT(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
		
		size_t exec_cnt = CPU->rcu.cur_cbs_cnt + CPU->rcu.next_cbs_cnt;
		
		if (exec_cnt < CRITICAL_THRESHOLD) {
			exec_cbs(&CPU->rcu.cur_cbs);
			exec_cbs(&CPU->rcu.next_cbs);	
		} else {
			/* 
			 * Getting overwhelmed with too many callbacks to run. 
			 * Disable preemption in order to prolong our time slice 
			 * and catch up with updaters posting new callbacks.
			 */
			preemption_disable();
			exec_cbs(&CPU->rcu.cur_cbs);
			exec_cbs(&CPU->rcu.next_cbs);	
			preemption_enable();
		}
		
		CPU->rcu.cur_cbs_cnt = 0;
		CPU->rcu.next_cbs_cnt = 0;
	} else if (CPU->rcu.cur_cbs_gp <= last_completed_gp) {

		if (CPU->rcu.cur_cbs_cnt < CRITICAL_THRESHOLD) {
			exec_cbs(&CPU->rcu.cur_cbs);
		} else {
			/* 
			 * Getting overwhelmed with too many callbacks to run. 
			 * Disable preemption in order to prolong our time slice 
			 * and catch up with updaters posting new callbacks.
			 */
			preemption_disable();
			exec_cbs(&CPU->rcu.cur_cbs);
			preemption_enable();
		}

		CPU->rcu.cur_cbs_cnt = 0;
	}
}

/** Executes callbacks in the single-linked list. The list is left empty. */
static void exec_cbs(rcu_item_t **phead)
{
	rcu_item_t *rcu_item = *phead;

	while (rcu_item) {
		/* func() may free rcu_item. Get a local copy. */
		rcu_item_t *next = rcu_item->next;
		rcu_func_t func = rcu_item->func;
		
		func(rcu_item);
		
		rcu_item = next;
	}
	
	*phead = 0;
}

static void upd_stat_cb_cnts(size_t arriving_cnt)
{
	CPU->rcu.stat_max_cbs = max(arriving_cnt, CPU->rcu.stat_max_cbs);
	if (0 < arriving_cnt) {
		CPU->rcu.stat_avg_cbs = 
			(99 * CPU->rcu.stat_avg_cbs + 1 * arriving_cnt) / 100;
	}
}


/** Prepares another batch of callbacks to dispatch at the nest grace period.
 * 
 * @return True if the next batch of callbacks must be expedited quickly.
 */
static bool advance_cbs(void)
{
	/* Move next_cbs to cur_cbs. */
	CPU->rcu.cur_cbs = CPU->rcu.next_cbs;
	CPU->rcu.cur_cbs_cnt = CPU->rcu.next_cbs_cnt;
	CPU->rcu.cur_cbs_gp = CPU->rcu.next_cbs_gp;
	
	/* Move arriving_cbs to next_cbs. Empties arriving_cbs. */
	ipl_t ipl = interrupts_disable();

	/* 
	 * Too many callbacks queued. Better speed up the detection
	 * or risk exhausting all system memory.
	 */
	bool expedite = (EXPEDITE_THRESHOLD < CPU->rcu.arriving_cbs_cnt)
		|| CPU->rcu.expedite_arriving;	

	CPU->rcu.expedite_arriving = false;
	
	CPU->rcu.next_cbs = CPU->rcu.arriving_cbs;
	CPU->rcu.next_cbs_cnt = CPU->rcu.arriving_cbs_cnt;
	
	CPU->rcu.arriving_cbs = 0;
	CPU->rcu.parriving_cbs_tail = &CPU->rcu.arriving_cbs;
	CPU->rcu.arriving_cbs_cnt = 0;
	
	interrupts_restore(ipl);

	/* Update statistics of arrived callbacks. */
	upd_stat_cb_cnts(CPU->rcu.next_cbs_cnt);
	
	/* 
	 * Make changes prior to queuing next_cbs visible to readers. 
	 * See comment in wait_for_readers().
	 */
	memory_barrier(); /* MB A, B */

	/* At the end of next_cbs_gp, exec next_cbs. Determine what GP that is. */
	
	if (!next_cbs_empty()) {
		spinlock_lock(&rcu.gp_lock);
	
		/* Exec next_cbs at the end of the next GP. */
		CPU->rcu.next_cbs_gp = rcu.cur_gp + 1;
		
		/* 
		 * There are no callbacks to invoke before next_cbs. Instruct
		 * wait_for_cur_cbs_gp() to notify us of the nearest GP end.
		 * That could be sooner than next_cbs_gp (if the current GP 
		 * had not yet completed), so we'll create a shorter batch
		 * of callbacks next time around.
		 */
		if (cur_cbs_empty()) {
			CPU->rcu.cur_cbs_gp = rcu.completed_gp + 1;
		} 
		
		spinlock_unlock(&rcu.gp_lock);
	} else {
		CPU->rcu.next_cbs_gp = CPU->rcu.cur_cbs_gp;
	}
	
	ASSERT(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
	
	return expedite;	
}

/** Waits for the grace period associated with callbacks cub_cbs to elapse. 
 * 
 * @param expedite Instructs the detector to aggressively speed up grace 
 *            period detection without any delay.
 * @param completed_gp Returns the most recent completed grace period 
 *            number.
 * @return false if the thread was interrupted and should stop.
 */
static bool wait_for_cur_cbs_gp_end(bool expedite, rcu_gp_t *completed_gp)
{
	/* 
	 * Use a possibly outdated version of completed_gp to bypass checking
	 * with the lock.
	 * 
	 * Note that loading and storing rcu.completed_gp is not atomic 
	 * (it is 64bit wide). Reading a clobbered value that is less than 
	 * rcu.completed_gp is harmless - we'll recheck with a lock. The 
	 * only way to read a clobbered value that is greater than the actual 
	 * value is if the detector increases the higher-order word first and 
	 * then decreases the lower-order word (or we see stores in that order), 
	 * eg when incrementing from 2^32 - 1 to 2^32. The loaded value 
	 * suddenly jumps by 2^32. It would take hours for such an increase 
	 * to occur so it is safe to discard the value. We allow increases 
	 * of up to half the maximum to generously accommodate for loading an
	 * outdated lower word.
	 */
	rcu_gp_t compl_gp = ACCESS_ONCE(rcu.completed_gp);
	if (CPU->rcu.cur_cbs_gp <= compl_gp 
		&& compl_gp <= CPU->rcu.cur_cbs_gp + UINT32_MAX_HALF) {
		*completed_gp = compl_gp;
		return true;
	}
	
	spinlock_lock(&rcu.gp_lock);
	
	if (CPU->rcu.cur_cbs_gp <= rcu.completed_gp) {
		*completed_gp = rcu.completed_gp;
		spinlock_unlock(&rcu.gp_lock);
		return true;
	}
	
	ASSERT(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
	ASSERT(rcu.cur_gp <= CPU->rcu.cur_cbs_gp);
	
	/* 
	 * Notify the detector of how many GP ends we intend to wait for, so 
	 * it can avoid going to sleep unnecessarily. Optimistically assume
	 * new callbacks will arrive while we're waiting; hence +1.
	 */
	size_t remaining_gp_ends = (size_t) (CPU->rcu.next_cbs_gp - rcu.cur_gp);
	req_detection(remaining_gp_ends + (arriving_cbs_empty() ? 0 : 1));
	
	/* 
	 * Ask the detector to speed up GP detection if there are too many 
	 * pending callbacks and other reclaimers have not already done so.
	 */
	if (expedite) {
		if(0 == rcu.req_expedited_cnt) 
			condvar_signal(&rcu.expedite_now);
		
		/* 
		 * Expedite only cub_cbs. If there really is a surge of callbacks 
		 * the arriving batch will expedite the GP for the huge number
		 * of callbacks currently in next_cbs
		 */
		rcu.req_expedited_cnt = 1;
	}

	/* Wait for cur_cbs_gp to end. */
	bool interrupted = cv_wait_for_gp(CPU->rcu.cur_cbs_gp);
	
	*completed_gp = rcu.completed_gp;
	spinlock_unlock(&rcu.gp_lock);	
	
	upd_missed_gp_in_wait(*completed_gp);
	
	return !interrupted;
}

static void upd_missed_gp_in_wait(rcu_gp_t completed_gp)
{
	ASSERT(CPU->rcu.cur_cbs_gp <= completed_gp);
	
	size_t delta = (size_t)(completed_gp - CPU->rcu.cur_cbs_gp);
	CPU->rcu.stat_missed_gp_in_wait += delta;
}


/** Requests the detector to detect at least req_cnt consecutive grace periods.*/
static void req_detection(size_t req_cnt)
{
	if (rcu.req_gp_end_cnt < req_cnt) {
		bool detector_idle = (0 == rcu.req_gp_end_cnt);
		rcu.req_gp_end_cnt = req_cnt;

		if (detector_idle) {
			ASSERT(rcu.cur_gp == rcu.completed_gp);
			condvar_signal(&rcu.req_gp_changed);
		}
	}
}

/** Waits for an announcement of the end of the grace period wait_on_gp. */
static bool cv_wait_for_gp(rcu_gp_t wait_on_gp)
{
	ASSERT(spinlock_locked(&rcu.gp_lock));
	
	bool interrupted = false;
	
	/* Wait until wait_on_gp ends. */
	while (rcu.completed_gp < wait_on_gp && !interrupted) {
		int ret = _condvar_wait_timeout_spinlock(&rcu.gp_ended, &rcu.gp_lock, 
			SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);
		interrupted = (ret == ESYNCH_INTERRUPTED);
	}
	
	ASSERT(wait_on_gp <= rcu.completed_gp);
	
	return interrupted;
}

/** The detector thread detects and notifies reclaimers of grace period ends. */
static void detector(void *arg)
{
	spinlock_lock(&rcu.gp_lock);
	
	while (wait_for_detect_req()) {
		/* 
		 * Announce new GP started. Readers start lazily acknowledging that
		 * they passed a QS.
		 */
		start_new_gp();
		
		spinlock_unlock(&rcu.gp_lock);
		
		if (!wait_for_readers()) 
			goto unlocked_out;
		
		spinlock_lock(&rcu.gp_lock);

		/* Notify reclaimers that they may now invoke queued callbacks. */
		end_cur_gp();
	}
	
	spinlock_unlock(&rcu.gp_lock);
	
unlocked_out:
	return;
}

/** Waits for a request from a reclaimer thread to detect a grace period. */
static bool wait_for_detect_req(void)
{
	ASSERT(spinlock_locked(&rcu.gp_lock));
	
	bool interrupted = false;
	
	while (0 == rcu.req_gp_end_cnt && !interrupted) {
		int ret = _condvar_wait_timeout_spinlock(&rcu.req_gp_changed, 
			&rcu.gp_lock, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);
		
		interrupted = (ret == ESYNCH_INTERRUPTED);
	}
	
	return !interrupted;
}

/** Announces the start of a new grace period for preexisting readers to ack. */
static void start_new_gp(void)
{
	ASSERT(spinlock_locked(&rcu.gp_lock));
	
	irq_spinlock_lock(&rcu.preempt_lock, true);
	
	/* Start a new GP. Announce to readers that a quiescent state is needed. */
	++rcu.cur_gp;
	
	/* 
	 * Readers preempted before the start of this GP (next_preempted)
	 * are preexisting readers now that a GP started and will hold up 
	 * the current GP until they exit their reader sections.
	 * 
	 * Preempted readers from the previous GP have finished so 
	 * cur_preempted is empty, but see comment in record_qs(). 
	 */
	list_concat(&rcu.cur_preempted, &rcu.next_preempted);
	
	irq_spinlock_unlock(&rcu.preempt_lock, true);
}

static void end_cur_gp(void)
{
	ASSERT(spinlock_locked(&rcu.gp_lock));
	
	rcu.completed_gp = rcu.cur_gp;
	--rcu.req_gp_end_cnt;
	
	condvar_broadcast(&rcu.gp_ended);
}

/** Waits for readers that started before the current GP started to finish. */
static bool wait_for_readers(void)
{
	DEFINE_CPU_MASK(reading_cpus);
	
	/* All running cpus have potential readers. */
	cpu_mask_active(reading_cpus);

	/*
	 * Ensure the announcement of the start of a new GP (ie up-to-date 
	 * cur_gp) propagates to cpus that are just coming out of idle 
	 * mode before we sample their idle state flag.
	 * 
	 * Cpus guarantee that after they set CPU->idle = true they will not
	 * execute any RCU reader sections without first setting idle to
	 * false and issuing a memory barrier. Therefore, if rm_quiescent_cpus()
	 * later on sees an idle cpu, but the cpu is just exiting its idle mode,
	 * the cpu must not have yet executed its memory barrier (otherwise
	 * it would pair up with this mem barrier and we would see idle == false).
	 * That memory barrier will pair up with the one below and ensure
	 * that a reader on the now-non-idle cpu will see the most current
	 * cur_gp. As a result, such a reader will never attempt to semaphore_up(
	 * pending_readers) during this GP, which allows the detector to
	 * ignore that cpu (the detector thinks it is idle). Moreover, any
	 * changes made by RCU updaters will have propagated to readers
	 * on the previously idle cpu -- again thanks to issuing a memory
	 * barrier after returning from idle mode.
	 * 
	 * idle -> non-idle cpu      | detector      | reclaimer
	 * ------------------------------------------------------
	 * rcu reader 1              |               | rcu_call()
	 * MB X                      |               |
	 * idle = true               |               | rcu_call() 
	 * (no rcu readers allowed ) |               | MB A in advance_cbs() 
	 * MB Y                      | (...)         | (...)
	 * (no rcu readers allowed)  |               | MB B in advance_cbs() 
	 * idle = false              | ++cur_gp      |
	 * (no rcu readers allowed)  | MB C          |
	 * MB Z                      | signal gp_end |
	 * rcu reader 2              |               | exec_cur_cbs()
	 * 
	 * 
	 * MB Y orders visibility of changes to idle for detector's sake.
	 * 
	 * MB Z pairs up with MB C. The cpu making a transition from idle 
	 * will see the most current value of cur_gp and will not attempt
	 * to notify the detector even if preempted during this GP.
	 * 
	 * MB Z pairs up with MB A from the previous batch. Updaters' changes
	 * are visible to reader 2 even when the detector thinks the cpu is idle 
	 * but it is not anymore.
	 * 
	 * MB X pairs up with MB B. Late mem accesses of reader 1 are contained
	 * and visible before idling and before any callbacks are executed 
	 * by reclaimers.
	 * 
	 * In summary, the detector does not know of or wait for reader 2, but
	 * it does not have to since it is a new reader that will not access
	 * data from previous GPs and will see any changes.
	 */
	memory_barrier(); /* MB C */
	
	/* 
	 * Give readers time to pass through a QS. Also, batch arriving 
	 * callbacks in order to amortize detection overhead.
	 */
	if (!gp_sleep())
		return false;
	
	/* Non-intrusively determine which cpus have yet to pass a QS. */
	rm_quiescent_cpus(reading_cpus);
	
	/* Actively interrupt cpus delaying the current GP and demand a QS. */
	interrupt_delaying_cpus(reading_cpus);
	
	/* Wait for the interrupted cpus to notify us that they reached a QS. */
	if (!wait_for_delaying_cpus())
		return false;
	/*
	 * All cpus recorded a QS or are still idle. Any new readers will be added
	 * to next_preempt if preempted, ie the number of readers in cur_preempted
	 * monotonically descreases.
	 */
	
	/* Wait for the last reader in cur_preempted to notify us it is done. */
	if (!wait_for_preempt_reader())
		return false;
	
	return true;
}

/** Remove those cpus from the mask that have already passed a quiescent
 * state since the start of the current grace period.
 */
static void rm_quiescent_cpus(cpu_mask_t *cpu_mask)
{
	cpu_mask_for_each(*cpu_mask, cpu_id) {
		/* 
		 * The cpu already checked for and passed through a quiescent 
		 * state since the beginning of this GP.
		 * 
		 * rcu.cur_gp is modified by local detector thread only. 
		 * Therefore, it is up-to-date even without a lock. 
		 */
		bool cpu_acked_gp = (cpus[cpu_id].rcu.last_seen_gp == rcu.cur_gp);
		
		/*
		 * Either the cpu is idle or it is exiting away from idle mode
		 * and already sees the most current rcu.cur_gp. See comment
		 * in wait_for_readers().
		 */
		bool cpu_idle = cpus[cpu_id].idle;
		
		if (cpu_acked_gp || cpu_idle) {
			cpu_mask_reset(cpu_mask, cpu_id);
		}
	}
}

/** Sleeps a while if the current grace period is not to be expedited. */
static bool gp_sleep(void)
{
	spinlock_lock(&rcu.gp_lock);

	int ret = 0;
	while (0 == rcu.req_expedited_cnt && 0 == ret) {
		/* minor bug: sleeps for the same duration if woken up spuriously. */
		ret = _condvar_wait_timeout_spinlock(&rcu.expedite_now, &rcu.gp_lock,
			DETECT_SLEEP_MS * 1000, SYNCH_FLAGS_INTERRUPTIBLE);
	}
	
	if (0 < rcu.req_expedited_cnt) {
		--rcu.req_expedited_cnt;
		/* Update statistic. */
		++rcu.stat_expedited_cnt;
	}
	
	spinlock_unlock(&rcu.gp_lock);
	
	return (ret != ESYNCH_INTERRUPTED);
}

/** Actively interrupts and checks the offending cpus for quiescent states. */
static void interrupt_delaying_cpus(cpu_mask_t *cpu_mask)
{
	const size_t max_conconcurrent_calls = 16;
	smp_call_t call[max_conconcurrent_calls];
	size_t outstanding_calls = 0;
	
	atomic_set(&rcu.delaying_cpu_cnt, 0);
	
	cpu_mask_for_each(*cpu_mask, cpu_id) {
		smp_call_async(cpu_id, sample_local_cpu, 0, &call[outstanding_calls]);
		++outstanding_calls;

		/* Update statistic. */
		if (CPU->id != cpu_id)
			++rcu.stat_smp_call_cnt;
		
		if (outstanding_calls == max_conconcurrent_calls) {
			for (size_t k = 0; k < outstanding_calls; ++k) {
				smp_call_wait(&call[k]);
			}
			
			outstanding_calls = 0;
		}
	}
	
	for (size_t k = 0; k < outstanding_calls; ++k) {
		smp_call_wait(&call[k]);
	}
}

/** Invoked on a cpu delaying grace period detection. 
 * 
 * Induces a quiescent state for the cpu or it instructs remaining 
 * readers to notify the detector once they finish.
 */
static void sample_local_cpu(void *arg)
{
	ASSERT(interrupts_disabled());
	ASSERT(!CPU->rcu.is_delaying_gp);
	
	/* Cpu did not pass a quiescent state yet. */
	if (CPU->rcu.last_seen_gp != rcu.cur_gp) {
		/* Interrupted a reader in a reader critical section. */
		if (0 < (*CPU->rcu.pnesting_cnt)) {
			ASSERT(!CPU->idle);
			/* Note to notify the detector from rcu_read_unlock(). */
			CPU->rcu.is_delaying_gp = true;
			atomic_inc(&rcu.delaying_cpu_cnt);
		} else {
			/* 
			 * The cpu did not enter any rcu reader sections since 
			 * the start of the current GP. Record a quiescent state.
			 * 
			 * Or, we interrupted rcu_read_unlock_impl() right before
			 * it recorded a QS. Record a QS for it. The memory barrier 
			 * contains the reader section's mem accesses before 
			 * updating last_seen_gp.
			 * 
			 * Or, we interrupted rcu_read_lock() right after it recorded
			 * a QS for the previous GP but before it got a chance to
			 * increment its nesting count. The memory barrier again
			 * stops the CS code from spilling out of the CS.
			 */
			memory_barrier();
			CPU->rcu.last_seen_gp = rcu.cur_gp;
		}
	} else {
		/* 
		 * This cpu already acknowledged that it had passed through 
		 * a quiescent state since the start of cur_gp. 
		 */
	}
	
	/* 
	 * smp_call() makes sure any changes propagate back to the caller.
	 * In particular, it makes the most current last_seen_gp visible
	 * to the detector.
	 */
}

/** Waits for cpus delaying the current grace period if there are any. */
static bool wait_for_delaying_cpus(void)
{
	int delaying_cpu_cnt = atomic_get(&rcu.delaying_cpu_cnt);

	for (int i = 0; i < delaying_cpu_cnt; ++i){
		if (!semaphore_down_interruptable(&rcu.remaining_readers))
			return false;
	}
	
	/* Update statistic. */
	rcu.stat_delayed_cnt += delaying_cpu_cnt;
	
	return true;
}

/** Waits for any preempted readers blocking this grace period to finish.*/
static bool wait_for_preempt_reader(void)
{
	irq_spinlock_lock(&rcu.preempt_lock, true);

	bool reader_exists = !list_empty(&rcu.cur_preempted);
	rcu.preempt_blocking_det = reader_exists;
	
	irq_spinlock_unlock(&rcu.preempt_lock, true);
	
	if (reader_exists) {
		/* Update statistic. */
		++rcu.stat_preempt_blocking_cnt;
		
		return semaphore_down_interruptable(&rcu.remaining_readers);
	} 	
	
	return true;
}

/** Called by the scheduler() when switching away from the current thread. */
void rcu_after_thread_ran(void)
{
	ASSERT(interrupts_disabled());
	ASSERT(CPU->rcu.pnesting_cnt == &THREAD->rcu.nesting_cnt);
	
	/* Preempted a reader critical section for the first time. */
	if (0 < THREAD->rcu.nesting_cnt && !THREAD->rcu.was_preempted) {
		THREAD->rcu.was_preempted = true;
		
		irq_spinlock_lock(&rcu.preempt_lock, false);
		
		if (CPU->rcu.last_seen_gp != rcu.cur_gp) {
			/* The reader started before the GP started - we must wait for it.*/
			list_append(&THREAD->rcu.preempt_link, &rcu.cur_preempted);
		} else {
			/* 
			 * The reader started after the GP started and this cpu
			 * already noted a quiescent state. We might block the next GP.
			 */
			list_append(&THREAD->rcu.preempt_link, &rcu.next_preempted);
		}

		irq_spinlock_unlock(&rcu.preempt_lock, false);
	}
	
	/* 
	 * The preempted reader has been noted globally. There are therefore
	 * no readers running on this cpu so this is a quiescent state.
	 */
	record_qs();

	/* 
	 * This cpu is holding up the current GP. Let the detector know 
	 * it has just passed a quiescent state. 
	 * 
	 * The detector waits separately for preempted readers, so we have 
	 * to notify the detector even if we have just preempted a reader.
	 */
	if (CPU->rcu.is_delaying_gp) {
		CPU->rcu.is_delaying_gp = false;
		semaphore_up(&rcu.remaining_readers);
	}
	
	/* 
	 * After this point THREAD is 0 and stays 0 until the scheduler()
	 * switches to a new thread. Use a temporary nesting counter for readers
	 * in handlers of interrupts that are raised while idle in the scheduler.
	 */
	CPU->rcu.pnesting_cnt = &CPU->rcu.tmp_nesting_cnt;

	/* 
	 * Forcefully associate the detector with the highest priority
	 * even if preempted due to its time slice running out.
	 * 
	 * todo: Replace with strict scheduler priority classes.
	 */
	if (THREAD == rcu.detector_thr) {
		THREAD->priority = -1;
	} 
	else if (THREAD == CPU->rcu.reclaimer_thr) {
		THREAD->priority = -1;
	} 
	
	upd_max_cbs_in_slice();
}

static void upd_max_cbs_in_slice(void)
{
	rcu_cpu_data_t *cr = &CPU->rcu;
	
	if (cr->arriving_cbs_cnt > cr->last_arriving_cnt) {
		size_t arrived_cnt = cr->arriving_cbs_cnt - cr->last_arriving_cnt;
		cr->stat_max_slice_cbs = max(arrived_cnt, cr->stat_max_slice_cbs);
	}
	
	cr->last_arriving_cnt = cr->arriving_cbs_cnt;
}

/** Called by the scheduler() when switching to a newly scheduled thread. */
void rcu_before_thread_runs(void)
{
	ASSERT(PREEMPTION_DISABLED || interrupts_disabled());
	ASSERT(&CPU->rcu.tmp_nesting_cnt == CPU->rcu.pnesting_cnt);
	
	CPU->rcu.pnesting_cnt = &THREAD->rcu.nesting_cnt;
}


/** Prints RCU run-time statistics. */
void rcu_print_stat(void)
{
	/* 
	 * Don't take locks. Worst case is we get out-dated values. 
	 * CPU local values are updated without any locks, so there 
	 * are no locks to lock in order to get up-to-date values.
	 */
	
	printf("Configuration: expedite_threshold=%d, critical_threshold=%d,"
		" detect_sleep=%dms\n",	
		EXPEDITE_THRESHOLD, CRITICAL_THRESHOLD, DETECT_SLEEP_MS);
	printf("Completed GPs: %" PRIu64 "\n", rcu.completed_gp);
	printf("Expedited GPs: %zu\n", rcu.stat_expedited_cnt);
	printf("Delayed GPs:   %zu (cpus w/ still running readers after gp sleep)\n", 
		rcu.stat_delayed_cnt);
	printf("Preempt blocked GPs: %zu (waited for preempted readers; "
		"running or not)\n", rcu.stat_preempt_blocking_cnt);
	printf("Smp calls:     %zu\n", rcu.stat_smp_call_cnt);
	
	printf("Max arrived callbacks per GP and CPU:\n");
	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		printf(" %zu", cpus[i].rcu.stat_max_cbs);
	}

	printf("\nAvg arrived callbacks per GP and CPU (nonempty batches only):\n");
	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		printf(" %zu", cpus[i].rcu.stat_avg_cbs);
	}
	
	printf("\nMax arrived callbacks per time slice and CPU:\n");
	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		printf(" %zu", cpus[i].rcu.stat_max_slice_cbs);
	}

	printf("\nMissed GP notifications per CPU:\n");
	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		printf(" %zu", cpus[i].rcu.stat_missed_gps);
	}

	printf("\nMissed GP notifications per CPU while waking up:\n");
	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		printf(" %zu", cpus[i].rcu.stat_missed_gp_in_wait);
	}
	printf("\n");
}

/** @}
 */
