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
 * 
 * @par Podzimek-preempt-RCU (RCU_PREEMPT_PODZIMEK)
 * 
 * Podzimek-preempt-RCU is a preemptible variant of Podzimek's non-preemptible
 * RCU algorithm [1, 2]. Grace period (GP) detection is centralized into a
 * single detector thread. The detector requests that each cpu announces
 * that it passed a quiescent state (QS), ie a state when the cpu is
 * outside of an rcu reader section (CS). Cpus check for QSs during context
 * switches and when entering and exiting rcu reader sections. Once all 
 * cpus announce a QS and if there were no threads preempted in a CS, the 
 * GP ends.
 * 
 * The detector increments the global GP counter, _rcu_cur_gp, in order 
 * to start a new GP. Readers notice the new GP by comparing the changed 
 * _rcu_cur_gp to a locally stored value last_seen_gp which denotes the
 * the last GP number for which the cpu noted an explicit QS (and issued
 * a memory barrier). Readers check for the change in the outer-most
 * (ie not nested) rcu_read_lock()/unlock() as these functions represent 
 * a QS. The reader first executes a memory barrier (MB) in order to contain 
 * memory references within a CS (and to make changes made by writers 
 * visible in the CS following rcu_read_lock()). Next, the reader notes 
 * that it reached a QS by updating the cpu local last_seen_gp to the
 * global GP counter, _rcu_cur_gp. Cache coherency eventually makes
 * the updated last_seen_gp visible to the detector cpu, much like it
 * delivered the changed _rcu_cur_gp to all cpus.
 * 
 * The detector waits a while after starting a GP and then reads each 
 * cpu's last_seen_gp to see if it reached a QS. If a cpu did not record 
 * a QS (might be a long running thread without an RCU reader CS; or cache
 * coherency has yet to make the most current last_seen_gp visible to
 * the detector; or the cpu is still in a CS) the cpu is interrupted
 * via an IPI. If the IPI handler finds the cpu still in a CS, it instructs
 * the cpu to notify the detector that it had exited the CS via a semaphore
 * (CPU->rcu.is_delaying_gp). 
 * The detector then waits on the semaphore for any cpus to exit their
 * CSs. Lastly, it waits for the last reader preempted in a CS to 
 * exit its CS if there were any and signals the end of the GP to
 * separate reclaimer threads wired to each cpu. Reclaimers then
 * execute the callbacks queued on each of the cpus.
 * 
 * 
 * @par A-RCU algorithm (RCU_PREEMPT_A)
 * 
 * A-RCU is based on the user space rcu algorithm in [3] utilizing signals
 * (urcu) and Podzimek's rcu [1]. Like in Podzimek's rcu, callbacks are 
 * executed by cpu-bound reclaimer threads. There is however no dedicated 
 * detector thread and the reclaimers take on the responsibilities of the 
 * detector when they need to start a new GP. A new GP is again announced 
 * and acknowledged with _rcu_cur_gp and the cpu local last_seen_gp. Unlike
 * Podzimek's rcu, cpus check explicitly for QS only during context switches. 
 * Like in urcu, rcu_read_lock()/unlock() only maintain the nesting count
 * and never issue any memory barriers. This makes rcu_read_lock()/unlock()
 * simple and fast.
 * 
 * If a new callback is queued for a reclaimer and no GP is in progress,
 * the reclaimer takes on the role of a detector. The detector increments 
 * _rcu_cur_gp in order to start a new GP. It waits a while to give cpus 
 * a chance to switch a context (a natural QS). Then, it examines each
 * non-idle cpu that has yet to pass a QS via an IPI. The IPI handler
 * sees the most current _rcu_cur_gp and last_seen_gp and notes a QS
 * with a memory barrier and an update to last_seen_gp. If the handler
 * finds the cpu in a CS it does nothing and let the detector poll/interrupt
 * the cpu again after a short sleep.
 * 
 * @par Caveats
 * 
 * last_seen_gp and _rcu_cur_gp are always 64bit variables and they
 * are read non-atomically on 32bit machines. Reading a clobbered
 * value of last_seen_gp or _rcu_cur_gp or writing a clobbered value
 * of _rcu_cur_gp to last_seen_gp will at worst force the detector
 * to unnecessarily interrupt a cpu. Interrupting a cpu makes the 
 * correct value of _rcu_cur_gp visible to the cpu and correctly
 * resets last_seen_gp in both algorithms.
 * 
 * 
 * 
 * [1] Read-copy-update for opensolaris,
 *     2010, Podzimek
 *     https://andrej.podzimek.org/thesis.pdf
 * 
 * [2] (podzimek-rcu) implementation file "rcu.patch"
 *     http://d3s.mff.cuni.cz/projects/operating_systems/rcu/rcu.patch
 * 
 * [3] User-level implementations of read-copy update,
 *     2012, appendix
 *     http://www.rdrop.com/users/paulmck/RCU/urcu-supp-accepted.2011.08.30a.pdf
 * 
 */

#include <assert.h>
#include <synch/rcu.h>
#include <synch/condvar.h>
#include <synch/semaphore.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
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

/** 
 * The current grace period number. Increases monotonically. 
 * Lock rcu.gp_lock or rcu.preempt_lock to get a current value.
 */
rcu_gp_t _rcu_cur_gp;

/** Global RCU data. */
typedef struct rcu_data {
	/** Detector uses so signal reclaimers that a grace period ended. */
	condvar_t gp_ended;
	/** Reclaimers use to notify the detector to accelerate GP detection. */
	condvar_t expedite_now;
	/** 
	 * Protects: req_gp_end_cnt, req_expedited_cnt, completed_gp, _rcu_cur_gp;
	 * or: completed_gp, _rcu_cur_gp
	 */
	SPINLOCK_DECLARE(gp_lock);
	/**
	 * The number of the most recently completed grace period. At most 
	 * one behind _rcu_cur_gp. If equal to _rcu_cur_gp, a grace period 
	 * detection is not in progress and the detector is idle.
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
	
#ifdef RCU_PREEMPT_A
	
	/** 
	 * The detector waits on this semaphore for any preempted readers 
	 * delaying the grace period once all cpus pass a quiescent state.
	 */
	semaphore_t remaining_readers;

#elif defined(RCU_PREEMPT_PODZIMEK)
	
	/** Reclaimers notify the detector when they request more grace periods.*/
	condvar_t req_gp_changed;
	/** Number of grace period ends the detector was requested to announce. */
	size_t req_gp_end_cnt;
	/** Number of consecutive grace periods to detect quickly and aggressively.*/
	size_t req_expedited_cnt;
	/** 
	 * Number of cpus with readers that are delaying the current GP.
	 * They will up() remaining_readers.
	 */
	atomic_t delaying_cpu_cnt;
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
#endif
	
	/** Excludes simultaneous rcu_barrier() calls. */
	mutex_t barrier_mtx;
	/** Number of cpus that we are waiting for to complete rcu_barrier(). */
	atomic_t barrier_wait_cnt;
	/** rcu_barrier() waits for the completion of barrier callbacks on this wq.*/
	waitq_t barrier_wq;
	
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

static void start_reclaimers(void);
static void synch_complete(rcu_item_t *rcu_item);
static inline void rcu_call_impl(bool expedite, rcu_item_t *rcu_item, 
	rcu_func_t func);
static void add_barrier_cb(void *arg);
static void barrier_complete(rcu_item_t *barrier_item);
static bool arriving_cbs_empty(void);
static bool next_cbs_empty(void);
static bool cur_cbs_empty(void);
static bool all_cbs_empty(void);
static void reclaimer(void *arg);
static bool wait_for_pending_cbs(void);
static bool advance_cbs(void);
static void exec_completed_cbs(rcu_gp_t last_completed_gp);
static void exec_cbs(rcu_item_t **phead);
static bool wait_for_cur_cbs_gp_end(bool expedite, rcu_gp_t *last_completed_gp);
static void upd_missed_gp_in_wait(rcu_gp_t completed_gp);

#ifdef RCU_PREEMPT_PODZIMEK
static void start_detector(void);
static void read_unlock_impl(size_t *pnesting_cnt);
static void req_detection(size_t req_cnt);
static bool cv_wait_for_gp(rcu_gp_t wait_on_gp);
static void detector(void *);
static bool wait_for_detect_req(void);
static void end_cur_gp(void);
static bool wait_for_readers(void);
static bool gp_sleep(void);
static void interrupt_delaying_cpus(cpu_mask_t *cpu_mask);
static bool wait_for_delaying_cpus(void);
#elif defined(RCU_PREEMPT_A)
static bool wait_for_readers(bool expedite);
static bool gp_sleep(bool *expedite);
#endif

static void start_new_gp(void);
static void rm_quiescent_cpus(cpu_mask_t *cpu_mask);
static void sample_cpus(cpu_mask_t *reader_cpus, void *arg);
static void sample_local_cpu(void *);
static bool wait_for_preempt_reader(void);
static void note_preempted_reader(void);
static void rm_preempted_reader(void);
static void upd_max_cbs_in_slice(size_t arriving_cbs_cnt);



/** Initializes global RCU structures. */
void rcu_init(void)
{
	condvar_initialize(&rcu.gp_ended);
	condvar_initialize(&rcu.expedite_now);

	spinlock_initialize(&rcu.gp_lock, "rcu.gp_lock");
	_rcu_cur_gp = 0;
	rcu.completed_gp = 0;
	
	irq_spinlock_initialize(&rcu.preempt_lock, "rcu.preempt_lock");
	list_initialize(&rcu.cur_preempted);
	list_initialize(&rcu.next_preempted);
	rcu.preempt_blocking_det = false;
	
	mutex_initialize(&rcu.barrier_mtx, MUTEX_PASSIVE);
	atomic_set(&rcu.barrier_wait_cnt, 0);
	waitq_initialize(&rcu.barrier_wq);

	semaphore_initialize(&rcu.remaining_readers, 0);
	
#ifdef RCU_PREEMPT_PODZIMEK
	condvar_initialize(&rcu.req_gp_changed);
	
	rcu.req_gp_end_cnt = 0;
	rcu.req_expedited_cnt = 0;
	atomic_set(&rcu.delaying_cpu_cnt, 0);
#endif
	
	rcu.detector_thr = NULL;
	
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

#ifdef RCU_PREEMPT_PODZIMEK
	CPU->rcu.nesting_cnt = 0;
	CPU->rcu.is_delaying_gp = false;
	CPU->rcu.signal_unlock = false;
#endif
	
	CPU->rcu.cur_cbs = NULL;
	CPU->rcu.cur_cbs_cnt = 0;
	CPU->rcu.next_cbs = NULL;
	CPU->rcu.next_cbs_cnt = 0;
	CPU->rcu.arriving_cbs = NULL;
	CPU->rcu.parriving_cbs_tail = &CPU->rcu.arriving_cbs;
	CPU->rcu.arriving_cbs_cnt = 0;

	CPU->rcu.cur_cbs_gp = 0;
	CPU->rcu.next_cbs_gp = 0;
	
	semaphore_initialize(&CPU->rcu.arrived_flag, 0);

	/* BSP creates reclaimer threads before AP's rcu_cpu_init() runs. */
	if (config.cpu_active == 1)
		CPU->rcu.reclaimer_thr = NULL;
	
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
#ifdef RCU_PREEMPT_PODZIMEK
	start_detector();
#endif
	
	start_reclaimers();
}

/** Initializes any per-thread RCU structures. */
void rcu_thread_init(thread_t *thread)
{
	thread->rcu.nesting_cnt = 0;

#ifdef RCU_PREEMPT_PODZIMEK
	thread->rcu.was_preempted = false;
#endif
	
	link_initialize(&thread->rcu.preempt_link);
}


/** Cleans up global RCU resources and stops dispatching callbacks. 
 * 
 * Call when shutting down the kernel. Outstanding callbacks will
 * not be processed. Instead they will linger forever.
 */
void rcu_stop(void)
{
	/* Stop and wait for reclaimers. */
	for (unsigned int cpu_id = 0; cpu_id < config.cpu_active; ++cpu_id) {
		assert(cpus[cpu_id].rcu.reclaimer_thr != NULL);
	
		if (cpus[cpu_id].rcu.reclaimer_thr) {
			thread_interrupt(cpus[cpu_id].rcu.reclaimer_thr);
			thread_join(cpus[cpu_id].rcu.reclaimer_thr);
			thread_detach(cpus[cpu_id].rcu.reclaimer_thr);
			cpus[cpu_id].rcu.reclaimer_thr = NULL;
		}
	}

#ifdef RCU_PREEMPT_PODZIMEK
	/* Stop the detector and wait. */
	if (rcu.detector_thr) {
		thread_interrupt(rcu.detector_thr);
		thread_join(rcu.detector_thr);
		thread_detach(rcu.detector_thr);
		rcu.detector_thr = NULL;
	}
#endif
}

/** Returns the number of elapsed grace periods since boot. */
uint64_t rcu_completed_gps(void)
{
	spinlock_lock(&rcu.gp_lock);
	uint64_t completed = rcu.completed_gp;
	spinlock_unlock(&rcu.gp_lock);
	
	return completed;
}

/** Creates and runs cpu-bound reclaimer threads. */
static void start_reclaimers(void)
{
	for (unsigned int cpu_id = 0; cpu_id < config.cpu_count; ++cpu_id) {
		char name[THREAD_NAME_BUFLEN] = {0};
		
		snprintf(name, THREAD_NAME_BUFLEN - 1, "rcu-rec/%u", cpu_id);
		
		cpus[cpu_id].rcu.reclaimer_thr = 
			thread_create(reclaimer, NULL, TASK, THREAD_FLAG_NONE, name);

		if (!cpus[cpu_id].rcu.reclaimer_thr) 
			panic("Failed to create RCU reclaimer thread on cpu%u.", cpu_id);

		thread_wire(cpus[cpu_id].rcu.reclaimer_thr, &cpus[cpu_id]);
		thread_ready(cpus[cpu_id].rcu.reclaimer_thr);
	}
}

#ifdef RCU_PREEMPT_PODZIMEK

/** Starts the detector thread. */
static void start_detector(void)
{
	rcu.detector_thr = 
		thread_create(detector, NULL, TASK, THREAD_FLAG_NONE, "rcu-det");
	
	if (!rcu.detector_thr) 
		panic("Failed to create RCU detector thread.");
	
	thread_ready(rcu.detector_thr);
}

/** Returns true if in an rcu reader section. */
bool rcu_read_locked(void)
{
	preemption_disable();
	bool locked = 0 < CPU->rcu.nesting_cnt;
	preemption_enable();
	
	return locked;
}

/** Unlocks the local reader section using the given nesting count. 
 * 
 * Preemption or interrupts must be disabled. 
 * 
 * @param pnesting_cnt Either &CPU->rcu.tmp_nesting_cnt or 
 *           THREAD->rcu.nesting_cnt.
 */
static void read_unlock_impl(size_t *pnesting_cnt)
{
	assert(PREEMPTION_DISABLED || interrupts_disabled());
	
	if (0 == --(*pnesting_cnt)) {
		_rcu_record_qs();
		
		/* 
		 * The thread was preempted while in a critical section or 
		 * the detector is eagerly waiting for this cpu's reader 
		 * to finish. 
		 * 
		 * Note that THREAD may be NULL in scheduler() and not just during boot.
		 */
		if ((THREAD && THREAD->rcu.was_preempted) || CPU->rcu.is_delaying_gp) {
			/* Rechecks with disabled interrupts. */
			_rcu_signal_read_unlock();
		}
	}
}

/** If necessary, signals the detector that we exited a reader section. */
void _rcu_signal_read_unlock(void)
{
	assert(PREEMPTION_DISABLED || interrupts_disabled());
	
	/*
	 * If an interrupt occurs here (even a NMI) it may beat us to
	 * resetting .is_delaying_gp or .was_preempted and up the semaphore
	 * for us.
	 */
	
	/* 
	 * If the detector is eagerly waiting for this cpu's reader to unlock,
	 * notify it that the reader did so.
	 */
	if (local_atomic_exchange(&CPU->rcu.is_delaying_gp, false)) {
		semaphore_up(&rcu.remaining_readers);
	}
	
	/*
	 * This reader was preempted while in a reader section.
	 * We might be holding up the current GP. Notify the
	 * detector if so.
	 */
	if (THREAD && local_atomic_exchange(&THREAD->rcu.was_preempted, false)) {
		assert(link_used(&THREAD->rcu.preempt_link));

		rm_preempted_reader();
	}
	
	/* If there was something to signal to the detector we have done so. */
	CPU->rcu.signal_unlock = false;
}

#endif /* RCU_PREEMPT_PODZIMEK */

typedef struct synch_item {
	waitq_t wq;
	rcu_item_t rcu_item;
} synch_item_t;

/** Blocks until all preexisting readers exit their critical sections. */
void rcu_synchronize(void)
{
	_rcu_synchronize(false);
}

/** Blocks until all preexisting readers exit their critical sections. */
void rcu_synchronize_expedite(void)
{
	_rcu_synchronize(true);
}

/** Blocks until all preexisting readers exit their critical sections. */
void _rcu_synchronize(bool expedite)
{
	/* Calling from a reader section will deadlock. */
	assert(!rcu_read_locked());
	
	synch_item_t completion; 

	waitq_initialize(&completion.wq);
	_rcu_call(expedite, &completion.rcu_item, synch_complete);
	waitq_sleep(&completion.wq);
}

/** rcu_synchronize's callback. */
static void synch_complete(rcu_item_t *rcu_item)
{
	synch_item_t *completion = member_to_inst(rcu_item, synch_item_t, rcu_item);
	assert(completion);
	waitq_wakeup(&completion->wq, WAKEUP_FIRST);
}

/** Waits for all outstanding rcu calls to complete. */
void rcu_barrier(void)
{
	/* 
	 * Serialize rcu_barrier() calls so we don't overwrite cpu.barrier_item
	 * currently in use by rcu_barrier().
	 */
	mutex_lock(&rcu.barrier_mtx);
	
	/* 
	 * Ensure we queue a barrier callback on all cpus before the already
	 * enqueued barrier callbacks start signaling completion.
	 */
	atomic_set(&rcu.barrier_wait_cnt, 1);

	DEFINE_CPU_MASK(cpu_mask);
	cpu_mask_active(cpu_mask);
	
	cpu_mask_for_each(*cpu_mask, cpu_id) {
		smp_call(cpu_id, add_barrier_cb, NULL);
	}
	
	if (0 < atomic_predec(&rcu.barrier_wait_cnt)) {
		waitq_sleep(&rcu.barrier_wq);
	}
	
	mutex_unlock(&rcu.barrier_mtx);
}

/** Issues a rcu_barrier() callback on the local cpu. 
 * 
 * Executed with interrupts disabled.  
 */
static void add_barrier_cb(void *arg)
{
	assert(interrupts_disabled() || PREEMPTION_DISABLED);
	atomic_inc(&rcu.barrier_wait_cnt);
	rcu_call(&CPU->rcu.barrier_item, barrier_complete);
}

/** Local cpu's rcu_barrier() completion callback. */
static void barrier_complete(rcu_item_t *barrier_item)
{
	/* Is this the last barrier callback completed? */
	if (0 == atomic_predec(&rcu.barrier_wait_cnt)) {
		/* Notify rcu_barrier() that we're done. */
		waitq_wakeup(&rcu.barrier_wq, WAKEUP_FIRST);
	}
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
	rcu_call_impl(false, rcu_item, func);
}

/** rcu_call() implementation. See rcu_call() for comments. */
void _rcu_call(bool expedite, rcu_item_t *rcu_item, rcu_func_t func)
{
	rcu_call_impl(expedite, rcu_item, func);
}

/** rcu_call() inline-able implementation. See rcu_call() for comments. */
static inline void rcu_call_impl(bool expedite, rcu_item_t *rcu_item, 
	rcu_func_t func)
{
	assert(rcu_item);
	
	rcu_item->func = func;
	rcu_item->next = NULL;
	
	preemption_disable();

	rcu_cpu_data_t *r = &CPU->rcu;

	rcu_item_t **prev_tail 
		= local_atomic_exchange(&r->parriving_cbs_tail, &rcu_item->next);
	*prev_tail = rcu_item;
	
	/* Approximate the number of callbacks present. */
	++r->arriving_cbs_cnt;
	
	if (expedite) {
		r->expedite_arriving = true;
	}
	
	bool first_cb = (prev_tail == &CPU->rcu.arriving_cbs);
	
	/* Added first callback - notify the reclaimer. */
	if (first_cb && !semaphore_count_get(&r->arrived_flag)) {
		semaphore_up(&r->arrived_flag);
	}
	
	preemption_enable();
}

static bool cur_cbs_empty(void)
{
	assert(THREAD && THREAD->wired);
	return NULL == CPU->rcu.cur_cbs;
}

static bool next_cbs_empty(void)
{
	assert(THREAD && THREAD->wired);
	return NULL == CPU->rcu.next_cbs;
}

/** Disable interrupts to get an up-to-date result. */
static bool arriving_cbs_empty(void)
{
	assert(THREAD && THREAD->wired);
	/* 
	 * Accessing with interrupts enabled may at worst lead to 
	 * a false negative if we race with a local interrupt handler.
	 */
	return NULL == CPU->rcu.arriving_cbs;
}

static bool all_cbs_empty(void)
{
	return cur_cbs_empty() && next_cbs_empty() && arriving_cbs_empty();
}


/** Reclaimer thread dispatches locally queued callbacks once a GP ends. */
static void reclaimer(void *arg)
{
	assert(THREAD && THREAD->wired);
	assert(THREAD == CPU->rcu.reclaimer_thr);

	rcu_gp_t last_compl_gp = 0;
	bool ok = true;
	
	while (ok && wait_for_pending_cbs()) {
		assert(CPU->rcu.reclaimer_thr == THREAD);
		
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
		assert(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
		
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
	
	*phead = NULL;
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
	
	/* Move arriving_cbs to next_cbs. */
	
	CPU->rcu.next_cbs_cnt = CPU->rcu.arriving_cbs_cnt;
	CPU->rcu.arriving_cbs_cnt = 0;
	
	/* 
	 * Too many callbacks queued. Better speed up the detection
	 * or risk exhausting all system memory.
	 */
	bool expedite = (EXPEDITE_THRESHOLD < CPU->rcu.next_cbs_cnt)
		|| CPU->rcu.expedite_arriving;	
	CPU->rcu.expedite_arriving = false;

	/* Start moving the arriving_cbs list to next_cbs. */
	CPU->rcu.next_cbs = CPU->rcu.arriving_cbs;
	
	/* 
	 * At least one callback arrived. The tail therefore does not point
	 * to the head of arriving_cbs and we can safely reset it to NULL.
	 */
	if (CPU->rcu.next_cbs) {
		assert(CPU->rcu.parriving_cbs_tail != &CPU->rcu.arriving_cbs);
		
		CPU->rcu.arriving_cbs = NULL;
		/* Reset arriving_cbs before updating the tail pointer. */
		compiler_barrier();
		/* Updating the tail pointer completes the move of arriving_cbs. */
		ACCESS_ONCE(CPU->rcu.parriving_cbs_tail) = &CPU->rcu.arriving_cbs;
	} else {
		/* 
		 * arriving_cbs was null and parriving_cbs_tail pointed to it 
		 * so leave it that way. Note that interrupt handlers may have
		 * added a callback in the meantime so it is not safe to reset
		 * arriving_cbs or parriving_cbs.
		 */
	}

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
		CPU->rcu.next_cbs_gp = _rcu_cur_gp + 1;
		
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
	
	assert(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
	
	return expedite;	
}


#ifdef RCU_PREEMPT_A

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
	spinlock_lock(&rcu.gp_lock);

	assert(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
	assert(CPU->rcu.cur_cbs_gp <= _rcu_cur_gp + 1);
	
	while (rcu.completed_gp < CPU->rcu.cur_cbs_gp) {
		/* GP has not yet started - start a new one. */
		if (rcu.completed_gp == _rcu_cur_gp) {
			start_new_gp();
			spinlock_unlock(&rcu.gp_lock);

			if (!wait_for_readers(expedite))
				return false;

			spinlock_lock(&rcu.gp_lock);
			/* Notify any reclaimers this GP had ended. */
			rcu.completed_gp = _rcu_cur_gp;
			condvar_broadcast(&rcu.gp_ended);
		} else {
			/* GP detection is in progress.*/ 
			
			if (expedite) 
				condvar_signal(&rcu.expedite_now);
			
			/* Wait for the GP to complete. */
			errno_t ret = _condvar_wait_timeout_spinlock(&rcu.gp_ended, &rcu.gp_lock, 
				SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);
			
			if (ret == EINTR) {
				spinlock_unlock(&rcu.gp_lock);
				return false;			
			}
		}
	}
	
	upd_missed_gp_in_wait(rcu.completed_gp);
	
	*completed_gp = rcu.completed_gp;
	spinlock_unlock(&rcu.gp_lock);
	
	return true;
}

static bool wait_for_readers(bool expedite)
{
	DEFINE_CPU_MASK(reader_cpus);
	
	cpu_mask_active(reader_cpus);
	rm_quiescent_cpus(reader_cpus);
	
	while (!cpu_mask_is_none(reader_cpus)) {
		/* Give cpus a chance to context switch (a QS) and batch callbacks. */
		if(!gp_sleep(&expedite)) 
			return false;
		
		rm_quiescent_cpus(reader_cpus);
		sample_cpus(reader_cpus, reader_cpus);
	}
	
	/* Update statistic. */
	if (expedite) {
		++rcu.stat_expedited_cnt;
	}
	
	/* 
	 * All cpus have passed through a QS and see the most recent _rcu_cur_gp.
	 * As a result newly preempted readers will associate with next_preempted
	 * and the number of old readers in cur_preempted will monotonically
	 * decrease. Wait for those old/preexisting readers.
	 */
	return wait_for_preempt_reader();
}

static bool gp_sleep(bool *expedite)
{
	if (*expedite) {
		scheduler();
		return true;
	} else {
		spinlock_lock(&rcu.gp_lock);

		errno_t ret = 0;
		ret = _condvar_wait_timeout_spinlock(&rcu.expedite_now, &rcu.gp_lock,
			DETECT_SLEEP_MS * 1000, SYNCH_FLAGS_INTERRUPTIBLE);

		/* rcu.expedite_now was signaled. */
		if (ret == EOK) {
			*expedite = true;
		}

		spinlock_unlock(&rcu.gp_lock);

		return (ret != EINTR);
	}
}

static void sample_local_cpu(void *arg)
{
	assert(interrupts_disabled());
	cpu_mask_t *reader_cpus = (cpu_mask_t *)arg;
	
	bool locked = RCU_CNT_INC <= THE->rcu_nesting;
	/* smp_call machinery makes the most current _rcu_cur_gp visible. */
	bool passed_qs = (CPU->rcu.last_seen_gp == _rcu_cur_gp);
		
	if (locked && !passed_qs) {
		/* 
		 * This cpu has not yet passed a quiescent state during this grace
		 * period and it is currently in a reader section. We'll have to
		 * try to sample this cpu again later.
		 */
	} else {
		/* Either not in a reader section or already passed a QS. */
		cpu_mask_reset(reader_cpus, CPU->id);
		/* Contain new reader sections and make prior changes visible to them.*/
		memory_barrier();
		CPU->rcu.last_seen_gp = _rcu_cur_gp;
	}
}

/** Called by the scheduler() when switching away from the current thread. */
void rcu_after_thread_ran(void)
{
	assert(interrupts_disabled());

	/* 
	 * In order not to worry about NMI seeing rcu_nesting change work 
	 * with a local copy.
	 */
	size_t nesting_cnt = local_atomic_exchange(&THE->rcu_nesting, 0);
	
	/* 
	 * Ensures NMIs see .rcu_nesting without the WAS_PREEMPTED mark and
	 * do not accidentally call rm_preempted_reader() from unlock().
	 */
	compiler_barrier();
	
	/* Preempted a reader critical section for the first time. */
	if (RCU_CNT_INC <= nesting_cnt && !(nesting_cnt & RCU_WAS_PREEMPTED)) {
		nesting_cnt |= RCU_WAS_PREEMPTED;
		note_preempted_reader();
	}
	
	/* Save the thread's nesting count when it is not running. */
	THREAD->rcu.nesting_cnt = nesting_cnt;

	if (CPU->rcu.last_seen_gp != _rcu_cur_gp) {
		/* 
		 * Contain any memory accesses of old readers before announcing a QS. 
		 * Also make changes from the previous GP visible to this cpu.
		 * Moreover it separates writing to last_seen_gp from 
		 * note_preempted_reader().
		 */
		memory_barrier();
		/* 
		 * The preempted reader has been noted globally. There are therefore
		 * no readers running on this cpu so this is a quiescent state.
		 * 
		 * Reading the multiword _rcu_cur_gp non-atomically is benign. 
		 * At worst, the read value will be different from the actual value.
		 * As a result, both the detector and this cpu will believe
		 * this cpu has not yet passed a QS although it really did.
		 * 
		 * Reloading _rcu_cur_gp is benign, because it cannot change
		 * until this cpu acknowledges it passed a QS by writing to
		 * last_seen_gp. Since interrupts are disabled, only this
		 * code may to so (IPIs won't get through).
		 */
		CPU->rcu.last_seen_gp = _rcu_cur_gp;
	}

	/* 
	 * Forcefully associate the reclaimer with the highest priority
	 * even if preempted due to its time slice running out.
	 */
	if (THREAD == CPU->rcu.reclaimer_thr) {
		THREAD->priority = -1;
	} 
	
	upd_max_cbs_in_slice(CPU->rcu.arriving_cbs_cnt);
}

/** Called by the scheduler() when switching to a newly scheduled thread. */
void rcu_before_thread_runs(void)
{
	assert(!rcu_read_locked());
	
	/* Load the thread's saved nesting count from before it was preempted. */
	THE->rcu_nesting = THREAD->rcu.nesting_cnt;
}

/** Called from scheduler() when exiting the current thread. 
 * 
 * Preemption or interrupts are disabled and the scheduler() already
 * switched away from the current thread, calling rcu_after_thread_ran().
 */
void rcu_thread_exiting(void)
{
	assert(THE->rcu_nesting == 0);
	
	/* 
	 * The thread forgot to exit its reader critical section. 
	 * It is a bug, but rather than letting the entire system lock up
	 * forcefully leave the reader section. The thread is not holding 
	 * any references anyway since it is exiting so it is safe.
	 */
	if (RCU_CNT_INC <= THREAD->rcu.nesting_cnt) {
		/* Emulate _rcu_preempted_unlock() with the proper nesting count. */
		if (THREAD->rcu.nesting_cnt & RCU_WAS_PREEMPTED) {
			rm_preempted_reader();
		}

		printf("Bug: thread (id %" PRIu64 " \"%s\") exited while in RCU read"
			" section.\n", THREAD->tid, THREAD->name);
	}
}

/** Returns true if in an rcu reader section. */
bool rcu_read_locked(void)
{
	return RCU_CNT_INC <= THE->rcu_nesting;
}

/** Invoked when a preempted reader finally exits its reader section. */
void _rcu_preempted_unlock(void)
{
	assert(0 == THE->rcu_nesting || RCU_WAS_PREEMPTED == THE->rcu_nesting);
	
	size_t prev = local_atomic_exchange(&THE->rcu_nesting, 0);
	if (prev == RCU_WAS_PREEMPTED) {
		/* 
		 * NMI handlers are never preempted but may call rm_preempted_reader()
		 * if a NMI occurred in _rcu_preempted_unlock() of a preempted thread.
		 * The only other rcu code that may have been interrupted by the NMI
		 * in _rcu_preempted_unlock() is: an IPI/sample_local_cpu() and
		 * the initial part of rcu_after_thread_ran().
		 * 
		 * rm_preempted_reader() will not deadlock because none of the locks
		 * it uses are locked in this case. Neither _rcu_preempted_unlock()
		 * nor sample_local_cpu() nor the initial part of rcu_after_thread_ran()
		 * acquire any locks.
		 */
		rm_preempted_reader();
	}
}

#elif defined(RCU_PREEMPT_PODZIMEK)

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
	
	assert(CPU->rcu.cur_cbs_gp <= CPU->rcu.next_cbs_gp);
	assert(_rcu_cur_gp <= CPU->rcu.cur_cbs_gp);
	
	/* 
	 * Notify the detector of how many GP ends we intend to wait for, so 
	 * it can avoid going to sleep unnecessarily. Optimistically assume
	 * new callbacks will arrive while we're waiting; hence +1.
	 */
	size_t remaining_gp_ends = (size_t) (CPU->rcu.next_cbs_gp - _rcu_cur_gp);
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
	
	if (!interrupted)
		upd_missed_gp_in_wait(*completed_gp);
	
	return !interrupted;
}

/** Waits for an announcement of the end of the grace period wait_on_gp. */
static bool cv_wait_for_gp(rcu_gp_t wait_on_gp)
{
	assert(spinlock_locked(&rcu.gp_lock));
	
	bool interrupted = false;
	
	/* Wait until wait_on_gp ends. */
	while (rcu.completed_gp < wait_on_gp && !interrupted) {
		int ret = _condvar_wait_timeout_spinlock(&rcu.gp_ended, &rcu.gp_lock, 
			SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);
		interrupted = (ret == EINTR);
	}
	
	return interrupted;
}

/** Requests the detector to detect at least req_cnt consecutive grace periods.*/
static void req_detection(size_t req_cnt)
{
	if (rcu.req_gp_end_cnt < req_cnt) {
		bool detector_idle = (0 == rcu.req_gp_end_cnt);
		rcu.req_gp_end_cnt = req_cnt;

		if (detector_idle) {
			assert(_rcu_cur_gp == rcu.completed_gp);
			condvar_signal(&rcu.req_gp_changed);
		}
	}
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
	assert(spinlock_locked(&rcu.gp_lock));
	
	bool interrupted = false;
	
	while (0 == rcu.req_gp_end_cnt && !interrupted) {
		int ret = _condvar_wait_timeout_spinlock(&rcu.req_gp_changed, 
			&rcu.gp_lock, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);
		
		interrupted = (ret == EINTR);
	}
	
	return !interrupted;
}


static void end_cur_gp(void)
{
	assert(spinlock_locked(&rcu.gp_lock));
	
	rcu.completed_gp = _rcu_cur_gp;
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
	
	return (ret != EINTR);
}

/** Actively interrupts and checks the offending cpus for quiescent states. */
static void interrupt_delaying_cpus(cpu_mask_t *cpu_mask)
{
	atomic_set(&rcu.delaying_cpu_cnt, 0);
	
	sample_cpus(cpu_mask, NULL);
}

/** Invoked on a cpu delaying grace period detection. 
 * 
 * Induces a quiescent state for the cpu or it instructs remaining 
 * readers to notify the detector once they finish.
 */
static void sample_local_cpu(void *arg)
{
	assert(interrupts_disabled());
	assert(!CPU->rcu.is_delaying_gp);
	
	/* Cpu did not pass a quiescent state yet. */
	if (CPU->rcu.last_seen_gp != _rcu_cur_gp) {
		/* Interrupted a reader in a reader critical section. */
		if (0 < CPU->rcu.nesting_cnt) {
			assert(!CPU->idle);
			/* 
			 * Note to notify the detector from rcu_read_unlock(). 
			 * 
			 * ACCESS_ONCE ensures the compiler writes to is_delaying_gp
			 * only after it determines that we are in a reader CS.
			 */
			ACCESS_ONCE(CPU->rcu.is_delaying_gp) = true;
			CPU->rcu.signal_unlock = true;
			
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
			CPU->rcu.last_seen_gp = _rcu_cur_gp;
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

/** Called by the scheduler() when switching away from the current thread. */
void rcu_after_thread_ran(void)
{
	assert(interrupts_disabled());

	/* 
	 * Prevent NMI handlers from interfering. The detector will be notified
	 * in this function if CPU->rcu.is_delaying_gp. The current thread is 
	 * no longer running so there is nothing else to signal to the detector.
	 */
	CPU->rcu.signal_unlock = false;
	/* 
	 * Separates clearing of .signal_unlock from accesses to 
	 * THREAD->rcu.was_preempted and CPU->rcu.nesting_cnt.
	 */
	compiler_barrier();
	
	/* Save the thread's nesting count when it is not running. */
	THREAD->rcu.nesting_cnt = CPU->rcu.nesting_cnt;
	
	/* Preempted a reader critical section for the first time. */
	if (0 < THREAD->rcu.nesting_cnt && !THREAD->rcu.was_preempted) {
		THREAD->rcu.was_preempted = true;
		note_preempted_reader();
	}
	
	/* 
	 * The preempted reader has been noted globally. There are therefore
	 * no readers running on this cpu so this is a quiescent state.
	 */
	_rcu_record_qs();

	/* 
	 * Interrupt handlers might use RCU while idle in scheduler(). 
	 * The preempted reader has been noted globally, so the handlers 
	 * may now start announcing quiescent states.
	 */
	CPU->rcu.nesting_cnt = 0;
	
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
	
	upd_max_cbs_in_slice(CPU->rcu.arriving_cbs_cnt);
}

/** Called by the scheduler() when switching to a newly scheduled thread. */
void rcu_before_thread_runs(void)
{
	assert(PREEMPTION_DISABLED || interrupts_disabled());
	assert(0 == CPU->rcu.nesting_cnt);
	
	/* Load the thread's saved nesting count from before it was preempted. */
	CPU->rcu.nesting_cnt = THREAD->rcu.nesting_cnt;
	
	/* 
	 * Ensures NMI see the proper nesting count before .signal_unlock.
	 * Otherwise the NMI may incorrectly signal that a preempted reader
	 * exited its reader section.
	 */
	compiler_barrier();
	
	/* 
	 * In the unlikely event that a NMI occurs between the loading of the 
	 * variables and setting signal_unlock, the NMI handler may invoke 
	 * rcu_read_unlock() and clear signal_unlock. In that case we will
	 * incorrectly overwrite signal_unlock from false to true. This event
	 * is benign and the next rcu_read_unlock() will at worst 
	 * needlessly invoke _rcu_signal_unlock().
	 */
	CPU->rcu.signal_unlock = THREAD->rcu.was_preempted || CPU->rcu.is_delaying_gp;
}

/** Called from scheduler() when exiting the current thread. 
 * 
 * Preemption or interrupts are disabled and the scheduler() already
 * switched away from the current thread, calling rcu_after_thread_ran().
 */
void rcu_thread_exiting(void)
{
	assert(THREAD != NULL);
	assert(THREAD->state == Exiting);
	assert(PREEMPTION_DISABLED || interrupts_disabled());
	
	/* 
	 * The thread forgot to exit its reader critical section. 
	 * It is a bug, but rather than letting the entire system lock up
	 * forcefully leave the reader section. The thread is not holding 
	 * any references anyway since it is exiting so it is safe.
	 */
	if (0 < THREAD->rcu.nesting_cnt) {
		THREAD->rcu.nesting_cnt = 1;
		read_unlock_impl(&THREAD->rcu.nesting_cnt);

		printf("Bug: thread (id %" PRIu64 " \"%s\") exited while in RCU read"
			" section.\n", THREAD->tid, THREAD->name);
	}
}


#endif /* RCU_PREEMPT_PODZIMEK */

/** Announces the start of a new grace period for preexisting readers to ack. */
static void start_new_gp(void)
{
	assert(spinlock_locked(&rcu.gp_lock));
	
	irq_spinlock_lock(&rcu.preempt_lock, true);
	
	/* Start a new GP. Announce to readers that a quiescent state is needed. */
	++_rcu_cur_gp;
	
	/* 
	 * Readers preempted before the start of this GP (next_preempted)
	 * are preexisting readers now that a GP started and will hold up 
	 * the current GP until they exit their reader sections.
	 * 
	 * Preempted readers from the previous GP have finished so 
	 * cur_preempted is empty, but see comment in _rcu_record_qs(). 
	 */
	list_concat(&rcu.cur_preempted, &rcu.next_preempted);
	
	irq_spinlock_unlock(&rcu.preempt_lock, true);
}

/** Remove those cpus from the mask that have already passed a quiescent
 * state since the start of the current grace period.
 */
static void rm_quiescent_cpus(cpu_mask_t *cpu_mask)
{
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
	
	cpu_mask_for_each(*cpu_mask, cpu_id) {
		/* 
		 * The cpu already checked for and passed through a quiescent 
		 * state since the beginning of this GP.
		 * 
		 * _rcu_cur_gp is modified by local detector thread only. 
		 * Therefore, it is up-to-date even without a lock. 
		 * 
		 * cpu.last_seen_gp may not be up-to-date. At worst, we will
		 * unnecessarily sample its last_seen_gp with a smp_call. 
		 */
		bool cpu_acked_gp = (cpus[cpu_id].rcu.last_seen_gp == _rcu_cur_gp);
		
		/*
		 * Either the cpu is idle or it is exiting away from idle mode
		 * and already sees the most current _rcu_cur_gp. See comment
		 * in wait_for_readers().
		 */
		bool cpu_idle = cpus[cpu_id].idle;
		
		if (cpu_acked_gp || cpu_idle) {
			cpu_mask_reset(cpu_mask, cpu_id);
		}
	}
}

/** Serially invokes sample_local_cpu(arg) on each cpu of reader_cpus. */
static void sample_cpus(cpu_mask_t *reader_cpus, void *arg)
{
	cpu_mask_for_each(*reader_cpus, cpu_id) {
		smp_call(cpu_id, sample_local_cpu, arg);

		/* Update statistic. */
		if (CPU->id != cpu_id)
			++rcu.stat_smp_call_cnt;
	}
}

static void upd_missed_gp_in_wait(rcu_gp_t completed_gp)
{
	assert(CPU->rcu.cur_cbs_gp <= completed_gp);
	
	size_t delta = (size_t)(completed_gp - CPU->rcu.cur_cbs_gp);
	CPU->rcu.stat_missed_gp_in_wait += delta;
}

/** Globally note that the current thread was preempted in a reader section. */
static void note_preempted_reader(void)
{
	irq_spinlock_lock(&rcu.preempt_lock, false);

	if (CPU->rcu.last_seen_gp != _rcu_cur_gp) {
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

/** Remove the current thread from the global list of preempted readers. */
static void rm_preempted_reader(void)
{
	irq_spinlock_lock(&rcu.preempt_lock, true);
	
	assert(link_used(&THREAD->rcu.preempt_link));

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

	irq_spinlock_unlock(&rcu.preempt_lock, true);
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

static void upd_max_cbs_in_slice(size_t arriving_cbs_cnt)
{
	rcu_cpu_data_t *cr = &CPU->rcu;
	
	if (arriving_cbs_cnt > cr->last_arriving_cnt) {
		size_t arrived_cnt = arriving_cbs_cnt - cr->last_arriving_cnt;
		cr->stat_max_slice_cbs = max(arrived_cnt, cr->stat_max_slice_cbs);
	}
	
	cr->last_arriving_cnt = arriving_cbs_cnt;
}

/** Prints RCU run-time statistics. */
void rcu_print_stat(void)
{
	/* 
	 * Don't take locks. Worst case is we get out-dated values. 
	 * CPU local values are updated without any locks, so there 
	 * are no locks to lock in order to get up-to-date values.
	 */
	
#ifdef RCU_PREEMPT_PODZIMEK
	const char *algo = "podzimek-preempt-rcu";
#elif defined(RCU_PREEMPT_A)
	const char *algo = "a-preempt-rcu";
#endif
	
	printf("Config: expedite_threshold=%d, critical_threshold=%d,"
		" detect_sleep=%dms, %s\n",	
		EXPEDITE_THRESHOLD, CRITICAL_THRESHOLD, DETECT_SLEEP_MS, algo);
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
