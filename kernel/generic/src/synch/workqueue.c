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
 * @brief Work queue/thread pool that automatically adjusts its size
 *        depending on the current load. Queued work functions may sleep..
 */

#include <assert.h>
#include <errno.h>
#include <synch/workqueue.h>
#include <synch/spinlock.h>
#include <synch/condvar.h>
#include <synch/mutex.h>
#include <proc/thread.h>
#include <config.h>
#include <arch.h>
#include <cpu.h>
#include <macros.h>

#define WORKQ_MAGIC      0xf00c1333U
#define WORK_ITEM_MAGIC  0xfeec1777U


struct work_queue {
	/*
	 * Protects everything except activate_worker.
	 * Must be acquired after any thread->locks.
	 */
	IRQ_SPINLOCK_DECLARE(lock);

	/* Activates a worker if new work arrives or if shutting down the queue. */
	condvar_t activate_worker;

	/* Queue of work_items ready to be dispatched. */
	list_t queue;

	/* List of worker threads. */
	list_t workers;

	/* Number of work items queued. */
	size_t item_cnt;

	/* Indicates the work queue is shutting down. */
	bool stopping;
	const char *name;

	/* Total number of created worker threads. */
	size_t cur_worker_cnt;
	/* Number of workers waiting for work to arrive. */
	size_t idle_worker_cnt;
	/* Number of idle workers signaled that have not yet been woken up. */
	size_t activate_pending;
	/* Number of blocked workers sleeping in work func() (ie not idle). */
	size_t blocked_worker_cnt;

	/* Number of pending signal_worker_op() operations. */
	size_t pending_op_cnt;

	link_t nb_link;

#ifdef CONFIG_DEBUG
	/* Magic cookie for integrity checks. Immutable. Accessed without lock. */
	uint32_t cookie;
#endif
};


/** Min number of idle workers to keep. */
static size_t min_worker_cnt;
/** Max total number of workers - be it blocked, idle, or active. */
static size_t max_worker_cnt;
/** Max number of concurrently running active workers, ie not blocked nor idle. */
static size_t max_concurrent_workers;
/** Max number of work items per active worker before a new worker is activated.*/
static const size_t max_items_per_worker = 8;

/** System wide work queue. */
static struct work_queue g_work_queue;

static int booting = true;


typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);
	condvar_t req_cv;
	thread_t *thread;
	list_t work_queues;
} nonblock_adder_t;

static nonblock_adder_t nonblock_adder;



/** Typedef a worker thread signaling operation prototype. */
typedef void (*signal_op_t)(struct work_queue *workq);


/* Fwd decl. */
static void workq_preinit(struct work_queue *workq, const char *name);
static bool add_worker(struct work_queue *workq);
static void interrupt_workers(struct work_queue *workq);
static void wait_for_workers(struct work_queue *workq);
static int _workq_enqueue(struct work_queue *workq, work_t *work_item,
    work_func_t func, bool can_block);
static void init_work_item(work_t *work_item, work_func_t func);
static signal_op_t signal_worker_logic(struct work_queue *workq, bool can_block);
static void worker_thread(void *arg);
static bool dequeue_work(struct work_queue *workq, work_t **pwork_item);
static bool worker_unnecessary(struct work_queue *workq);
static void cv_wait(struct work_queue *workq);
static void nonblock_init(void);

#ifdef CONFIG_DEBUG
static bool workq_corrupted(struct work_queue *workq);
static bool work_item_corrupted(work_t *work_item);
#endif

/** Creates worker thread for the system-wide worker queue. */
void workq_global_worker_init(void)
{
	/*
	 * No need for additional synchronization. Stores to word-sized
	 * variables are atomic and the change will eventually propagate.
	 * Moreover add_worker() includes the necessary memory barriers
	 * in spinlock lock/unlock().
	 */
	booting = false;

	nonblock_init();

	if (!add_worker(&g_work_queue))
		panic("Could not create a single global work queue worker!\n");

}

/** Initializes the system wide work queue and support for other work queues. */
void workq_global_init(void)
{
	/* Keep idle workers on 1/4-th of cpus, but at least 2 threads. */
	min_worker_cnt = max(2, config.cpu_count / 4);
	/* Allow max 8 sleeping work items per cpu. */
	max_worker_cnt = max(32, 8 * config.cpu_count);
	/* Maximum concurrency without slowing down the system. */
	max_concurrent_workers = max(2, config.cpu_count);

	workq_preinit(&g_work_queue, "kworkq");
}

/** Stops the system global work queue and waits for all work items to complete.*/
void workq_global_stop(void)
{
	workq_stop(&g_work_queue);
}

/** Creates and initializes a work queue. Returns NULL upon failure. */
struct work_queue *workq_create(const char *name)
{
	struct work_queue *workq = malloc(sizeof(struct work_queue), 0);

	if (workq) {
		if (workq_init(workq, name)) {
			assert(!workq_corrupted(workq));
			return workq;
		}

		free(workq);
	}

	return NULL;
}

/** Frees work queue resources and stops it if it had not been done so already.*/
void workq_destroy(struct work_queue *workq)
{
	assert(!workq_corrupted(workq));

	irq_spinlock_lock(&workq->lock, true);
	bool stopped = workq->stopping;
#ifdef CONFIG_DEBUG
	size_t running_workers = workq->cur_worker_cnt;
#endif
	irq_spinlock_unlock(&workq->lock, true);

	if (!stopped) {
		workq_stop(workq);
	} else {
		assert(0 == running_workers);
	}

#ifdef CONFIG_DEBUG
	workq->cookie = 0;
#endif

	free(workq);
}

/** Initializes workq structure without creating any workers. */
static void workq_preinit(struct work_queue *workq, const char *name)
{
#ifdef CONFIG_DEBUG
	workq->cookie = WORKQ_MAGIC;
#endif

	irq_spinlock_initialize(&workq->lock, name);
	condvar_initialize(&workq->activate_worker);

	list_initialize(&workq->queue);
	list_initialize(&workq->workers);

	workq->item_cnt = 0;
	workq->stopping = false;
	workq->name = name;

	workq->cur_worker_cnt = 1;
	workq->idle_worker_cnt = 0;
	workq->activate_pending = 0;
	workq->blocked_worker_cnt = 0;

	workq->pending_op_cnt = 0;
	link_initialize(&workq->nb_link);
}

/** Initializes a work queue. Returns true if successful.
 *
 * Before destroying a work queue it must be stopped via
 * workq_stop().
 */
bool workq_init(struct work_queue *workq, const char *name)
{
	workq_preinit(workq, name);
	return add_worker(workq);
}

/** Add a new worker thread. Returns false if the thread could not be created. */
static bool add_worker(struct work_queue *workq)
{
	assert(!workq_corrupted(workq));

	thread_t *thread = thread_create(worker_thread, workq, TASK,
	    THREAD_FLAG_NONE, workq->name);

	if (!thread) {
		irq_spinlock_lock(&workq->lock, true);

		/* cur_worker_cnt proactively increased in signal_worker_logic() .*/
		assert(0 < workq->cur_worker_cnt);
		--workq->cur_worker_cnt;

		irq_spinlock_unlock(&workq->lock, true);
		return false;
	}

	/* Respect lock ordering. */
	irq_spinlock_lock(&thread->lock, true);
	irq_spinlock_lock(&workq->lock, false);

	bool success;

	if (!workq->stopping) {
		success = true;

		/* Try to distribute workers among cpus right away. */
		unsigned int cpu_id = (workq->cur_worker_cnt) % config.cpu_active;

		if (!cpus[cpu_id].active)
			cpu_id = CPU->id;

		thread->workq = workq;
		thread->cpu = &cpus[cpu_id];
		thread->workq_blocked = false;
		thread->workq_idling = false;
		link_initialize(&thread->workq_link);

		list_append(&thread->workq_link, &workq->workers);
	} else {
		/*
		 * Work queue is shutting down - we must not add the worker
		 * and we cannot destroy it without ready-ing it. Mark it
		 * interrupted so the worker exits right away without even
		 * touching workq.
		 */
		success = false;

		/* cur_worker_cnt proactively increased in signal_worker() .*/
		assert(0 < workq->cur_worker_cnt);
		--workq->cur_worker_cnt;
	}

	irq_spinlock_unlock(&workq->lock, false);
	irq_spinlock_unlock(&thread->lock, true);

	if (!success) {
		thread_interrupt(thread);
	}

	thread_ready(thread);

	return success;
}

/** Shuts down the work queue. Waits for all pending work items to complete.
 *
 * workq_stop() may only be run once.
 */
void workq_stop(struct work_queue *workq)
{
	assert(!workq_corrupted(workq));

	interrupt_workers(workq);
	wait_for_workers(workq);
}

/** Notifies worker threads the work queue is shutting down. */
static void interrupt_workers(struct work_queue *workq)
{
	irq_spinlock_lock(&workq->lock, true);

	/* workq_stop() may only be called once. */
	assert(!workq->stopping);
	workq->stopping = true;

	/* Respect lock ordering - do not hold workq->lock during broadcast. */
	irq_spinlock_unlock(&workq->lock, true);

	condvar_broadcast(&workq->activate_worker);
}

/** Waits for all worker threads to exit. */
static void wait_for_workers(struct work_queue *workq)
{
	assert(!PREEMPTION_DISABLED);

	irq_spinlock_lock(&workq->lock, true);

	list_foreach_safe(workq->workers, cur_worker, next_worker) {
		thread_t *worker = list_get_instance(cur_worker, thread_t, workq_link);
		list_remove(cur_worker);

		/* Wait without the lock. */
		irq_spinlock_unlock(&workq->lock, true);

		thread_join(worker);
		thread_detach(worker);

		irq_spinlock_lock(&workq->lock, true);
	}

	assert(list_empty(&workq->workers));

	/* Wait for deferred add_worker_op(), signal_worker_op() to finish. */
	while (0 < workq->cur_worker_cnt || 0 < workq->pending_op_cnt) {
		irq_spinlock_unlock(&workq->lock, true);

		scheduler();

		irq_spinlock_lock(&workq->lock, true);
	}

	irq_spinlock_unlock(&workq->lock, true);
}

/** Queues a function into the global wait queue without blocking.
 *
 * See workq_enqueue_noblock() for more details.
 */
bool workq_global_enqueue_noblock(work_t *work_item, work_func_t func)
{
	return workq_enqueue_noblock(&g_work_queue, work_item, func);
}

/** Queues a function into the global wait queue; may block.
 *
 * See workq_enqueue() for more details.
 */
bool workq_global_enqueue(work_t *work_item, work_func_t func)
{
	return workq_enqueue(&g_work_queue, work_item, func);
}

/** Adds a function to be invoked in a separate thread without blocking.
 *
 * workq_enqueue_noblock() is guaranteed not to block. It is safe
 * to invoke from interrupt handlers.
 *
 * Consider using workq_enqueue() instead if at all possible. Otherwise,
 * your work item may have to wait for previously enqueued sleeping
 * work items to complete if you are unlucky.
 *
 * @param workq     Work queue where to queue the work item.
 * @param work_item Work item bookkeeping structure. Must be valid
 *                  until func() is entered.
 * @param func      User supplied function to invoke in a worker thread.

 * @return false if work queue is shutting down; function is not
 *               queued for further processing.
 * @return true  Otherwise. func() will be invoked in a separate thread.
 */
bool workq_enqueue_noblock(struct work_queue *workq, work_t *work_item,
    work_func_t func)
{
	return _workq_enqueue(workq, work_item, func, false);
}

/** Adds a function to be invoked in a separate thread; may block.
 *
 * While the workq_enqueue() is unlikely to block, it may do so if too
 * many previous work items blocked sleeping.
 *
 * @param workq     Work queue where to queue the work item.
 * @param work_item Work item bookkeeping structure. Must be valid
 *                  until func() is entered.
 * @param func      User supplied function to invoke in a worker thread.

 * @return false if work queue is shutting down; function is not
 *               queued for further processing.
 * @return true  Otherwise. func() will be invoked in a separate thread.
 */
bool workq_enqueue(struct work_queue *workq, work_t *work_item, work_func_t func)
{
	return _workq_enqueue(workq, work_item, func, true);
}

/** Adds a work item that will be processed by a separate worker thread.
 *
 * func() will be invoked in another kernel thread and may block.
 *
 * Prefer to call _workq_enqueue() with can_block set. Otherwise
 * your work item may have to wait for sleeping work items to complete.
 * If all worker threads are blocked/sleeping a new worker thread cannot
 * be create without can_block set because creating a thread might
 * block due to low memory conditions.
 *
 * @param workq     Work queue where to queue the work item.
 * @param work_item Work item bookkeeping structure. Must be valid
 *                  until func() is entered.
 * @param func      User supplied function to invoke in a worker thread.
 * @param can_block May adding this work item block?

 * @return false if work queue is shutting down; function is not
 *               queued for further processing.
 * @return true  Otherwise.
 */
static int _workq_enqueue(struct work_queue *workq, work_t *work_item,
    work_func_t func, bool can_block)
{
	assert(!workq_corrupted(workq));

	bool success = true;
	signal_op_t signal_op = NULL;

	irq_spinlock_lock(&workq->lock, true);

	if (workq->stopping) {
		success = false;
	} else {
		init_work_item(work_item, func);
		list_append(&work_item->queue_link, &workq->queue);
		++workq->item_cnt;
		success = true;

		if (!booting) {
			signal_op = signal_worker_logic(workq, can_block);
		} else {
			/*
			 * During boot there are no workers to signal. Just queue
			 * the work and let future workers take care of it.
			 */
		}
	}

	irq_spinlock_unlock(&workq->lock, true);

	if (signal_op) {
		signal_op(workq);
	}

	return success;
}

/** Prepare an item to be added to the work item queue. */
static void init_work_item(work_t *work_item, work_func_t func)
{
#ifdef CONFIG_DEBUG
	work_item->cookie = WORK_ITEM_MAGIC;
#endif

	link_initialize(&work_item->queue_link);
	work_item->func = func;
}

/** Returns the number of workers running work func() that are not blocked. */
static size_t active_workers_now(struct work_queue *workq)
{
	assert(irq_spinlock_locked(&workq->lock));

	/* Workers blocked are sleeping in the work function (ie not idle). */
	assert(workq->blocked_worker_cnt <= workq->cur_worker_cnt);
	/* Idle workers are waiting for more work to arrive in condvar_wait. */
	assert(workq->idle_worker_cnt <= workq->cur_worker_cnt);

	/* Idle + blocked workers == sleeping worker threads. */
	size_t sleeping_workers = workq->blocked_worker_cnt + workq->idle_worker_cnt;

	assert(sleeping_workers	<= workq->cur_worker_cnt);
	/* Workers pending activation are idle workers not yet given a time slice. */
	assert(workq->activate_pending <= workq->idle_worker_cnt);

	/*
	 * Workers actively running the work func() this very moment and
	 * are neither blocked nor idle. Exclude ->activate_pending workers
	 * since they will run their work func() once they get a time slice
	 * and are not running it right now.
	 */
	return workq->cur_worker_cnt - sleeping_workers;
}

/**
 * Returns the number of workers that are running or are about to run work
 * func() and that are not blocked.
 */
static size_t active_workers(struct work_queue *workq)
{
	assert(irq_spinlock_locked(&workq->lock));

	/*
	 * Workers actively running the work func() and are neither blocked nor
	 * idle. ->activate_pending workers will run their work func() once they
	 * get a time slice after waking from a condvar wait, so count them
	 * as well.
	 */
	return active_workers_now(workq) + workq->activate_pending;
}

static void add_worker_noblock_op(struct work_queue *workq)
{
	condvar_signal(&nonblock_adder.req_cv);
}

static void add_worker_op(struct work_queue *workq)
{
	add_worker(workq);
}

static void signal_worker_op(struct work_queue *workq)
{
	assert(!workq_corrupted(workq));

	condvar_signal(&workq->activate_worker);

	irq_spinlock_lock(&workq->lock, true);
	assert(0 < workq->pending_op_cnt);
	--workq->pending_op_cnt;
	irq_spinlock_unlock(&workq->lock, true);
}

/** Determines how to signal workers if at all.
 *
 * @param workq     Work queue where a new work item was queued.
 * @param can_block True if we may block while signaling a worker or creating
 *                  a new worker.
 *
 * @return Function that will notify workers or NULL if no action is needed.
 */
static signal_op_t signal_worker_logic(struct work_queue *workq, bool can_block)
{
	assert(!workq_corrupted(workq));
	assert(irq_spinlock_locked(&workq->lock));

	/* Only signal workers if really necessary. */
	signal_op_t signal_op = NULL;

	/*
	 * Workers actively running the work func() and neither blocked nor idle.
	 * Including ->activate_pending workers that will run their work func()
	 * once they get a time slice.
	 */
	size_t active = active_workers(workq);
	/* Max total allowed number of work items queued for active workers. */
	size_t max_load = active * max_items_per_worker;

	/* Active workers are getting overwhelmed - activate another. */
	if (max_load < workq->item_cnt) {

		size_t remaining_idle =
		    workq->idle_worker_cnt - workq->activate_pending;

		/* Idle workers still exist - activate one. */
		if (remaining_idle > 0) {
			/*
			 * Directly changing idle_worker_cnt here would not allow
			 * workers to recognize spurious wake-ups. Change
			 * activate_pending instead.
			 */
			++workq->activate_pending;
			++workq->pending_op_cnt;
			signal_op = signal_worker_op;
		} else {
			/* No idle workers remain. Request that a new one be created. */
			bool need_worker = (active < max_concurrent_workers) &&
			    (workq->cur_worker_cnt < max_worker_cnt);

			if (need_worker && can_block) {
				signal_op = add_worker_op;
				/*
				 * It may take some time to actually create the worker.
				 * We don't want to swamp the thread pool with superfluous
				 * worker creation requests so pretend it was already
				 * created and proactively increase the worker count.
				 */
				++workq->cur_worker_cnt;
			}

			/*
			 * We cannot create a new worker but we need one desperately
			 * because all workers are blocked in their work functions.
			 */
			if (need_worker && !can_block && 0 == active) {
				assert(0 == workq->idle_worker_cnt);

				irq_spinlock_lock(&nonblock_adder.lock, true);

				if (nonblock_adder.thread && !link_used(&workq->nb_link)) {
					signal_op = add_worker_noblock_op;
					++workq->cur_worker_cnt;
					list_append(&workq->nb_link, &nonblock_adder.work_queues);
				}

				irq_spinlock_unlock(&nonblock_adder.lock, true);
			}
		}
	} else {
		/*
		 * There are enough active/running workers to process the queue.
		 * No need to signal/activate any new workers.
		 */
		signal_op = NULL;
	}

	return signal_op;
}

/** Executes queued work items. */
static void worker_thread(void *arg)
{
	/*
	 * The thread has been created after the work queue was ordered to stop.
	 * Do not access the work queue and return immediately.
	 */
	if (thread_interrupted(THREAD)) {
		thread_detach(THREAD);
		return;
	}

	assert(arg != NULL);

	struct work_queue *workq = arg;
	work_t *work_item;

	while (dequeue_work(workq, &work_item)) {
		/* Copy the func field so func() can safely free work_item. */
		work_func_t func = work_item->func;

		func(work_item);
	}
}

/** Waits and retrieves a work item. Returns false if the worker should exit. */
static bool dequeue_work(struct work_queue *workq, work_t **pwork_item)
{
	assert(!workq_corrupted(workq));

	irq_spinlock_lock(&workq->lock, true);

	/* Check if we should exit if load is low. */
	if (!workq->stopping && worker_unnecessary(workq)) {
		/* There are too many workers for this load. Exit. */
		assert(0 < workq->cur_worker_cnt);
		--workq->cur_worker_cnt;
		list_remove(&THREAD->workq_link);
		irq_spinlock_unlock(&workq->lock, true);

		thread_detach(THREAD);
		return false;
	}

	bool stop = false;

	/* Wait for work to arrive. */
	while (list_empty(&workq->queue) && !workq->stopping) {
		cv_wait(workq);

		if (0 < workq->activate_pending)
			--workq->activate_pending;
	}

	/* Process remaining work even if requested to stop. */
	if (!list_empty(&workq->queue)) {
		link_t *work_link = list_first(&workq->queue);
		*pwork_item = list_get_instance(work_link, work_t, queue_link);

#ifdef CONFIG_DEBUG
		assert(!work_item_corrupted(*pwork_item));
		(*pwork_item)->cookie = 0;
#endif
		list_remove(work_link);
		--workq->item_cnt;

		stop = false;
	} else {
		/* Requested to stop and no more work queued. */
		assert(workq->stopping);
		--workq->cur_worker_cnt;
		stop = true;
	}

	irq_spinlock_unlock(&workq->lock, true);

	return !stop;
}

/** Returns true if for the given load there are too many workers. */
static bool worker_unnecessary(struct work_queue *workq)
{
	assert(irq_spinlock_locked(&workq->lock));

	/* No work is pending. We don't need too many idle threads. */
	if (list_empty(&workq->queue)) {
		/* There are too many idle workers. Exit. */
		return (min_worker_cnt <= workq->idle_worker_cnt);
	} else {
		/*
		 * There is work but we are swamped with too many active workers
		 * that were woken up from sleep at around the same time. We
		 * don't need another worker fighting for cpu time.
		 */
		size_t active = active_workers_now(workq);
		return (max_concurrent_workers < active);
	}
}

/** Waits for a signal to activate_worker. Thread marked idle while waiting. */
static void cv_wait(struct work_queue *workq)
{
	++workq->idle_worker_cnt;
	THREAD->workq_idling = true;

	/* Ignore lock ordering just here. */
	assert(irq_spinlock_locked(&workq->lock));

	_condvar_wait_timeout_irq_spinlock(&workq->activate_worker,
	    &workq->lock, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);

	assert(!workq_corrupted(workq));
	assert(irq_spinlock_locked(&workq->lock));

	THREAD->workq_idling = false;
	--workq->idle_worker_cnt;
}


/** Invoked from thread_ready() right before the thread is woken up. */
void workq_before_thread_is_ready(thread_t *thread)
{
	assert(thread);
	assert(irq_spinlock_locked(&thread->lock));

	/* Worker's work func() is about to wake up from sleeping. */
	if (thread->workq && thread->workq_blocked) {
		/* Must be blocked in user work func() and not be waiting for work. */
		assert(!thread->workq_idling);
		assert(thread->state == Sleeping);
		assert(THREAD != thread);
		assert(!workq_corrupted(thread->workq));

		/* Protected by thread->lock */
		thread->workq_blocked = false;

		irq_spinlock_lock(&thread->workq->lock, true);
		--thread->workq->blocked_worker_cnt;
		irq_spinlock_unlock(&thread->workq->lock, true);
	}
}

/** Invoked from scheduler() before switching away from a thread. */
void workq_after_thread_ran(void)
{
	assert(THREAD);
	assert(irq_spinlock_locked(&THREAD->lock));

	/* Worker's work func() is about to sleep/block. */
	if (THREAD->workq && THREAD->state == Sleeping && !THREAD->workq_idling) {
		assert(!THREAD->workq_blocked);
		assert(!workq_corrupted(THREAD->workq));

		THREAD->workq_blocked = true;

		irq_spinlock_lock(&THREAD->workq->lock, false);

		++THREAD->workq->blocked_worker_cnt;

		bool can_block = false;
		signal_op_t op = signal_worker_logic(THREAD->workq, can_block);

		irq_spinlock_unlock(&THREAD->workq->lock, false);

		if (op) {
			assert(add_worker_noblock_op == op || signal_worker_op == op);
			op(THREAD->workq);
		}
	}
}

/** Prints stats of the work queue to the kernel console. */
void workq_print_info(struct work_queue *workq)
{
	irq_spinlock_lock(&workq->lock, true);

	size_t total = workq->cur_worker_cnt;
	size_t blocked = workq->blocked_worker_cnt;
	size_t idle = workq->idle_worker_cnt;
	size_t active = active_workers(workq);
	size_t items = workq->item_cnt;
	bool stopping = workq->stopping;
	bool worker_surplus = worker_unnecessary(workq);
	const char *load_str = worker_surplus ? "decreasing" :
	    (0 < workq->activate_pending) ? "increasing" : "stable";

	irq_spinlock_unlock(&workq->lock, true);

	printf(
	    "Configuration: max_worker_cnt=%zu, min_worker_cnt=%zu,\n"
	    " max_concurrent_workers=%zu, max_items_per_worker=%zu\n"
	    "Workers: %zu\n"
	    "Active:  %zu (workers currently processing work)\n"
	    "Blocked: %zu (work functions sleeping/blocked)\n"
	    "Idle:    %zu (idle workers waiting for more work)\n"
	    "Items:   %zu (queued not yet dispatched work)\n"
	    "Stopping: %d\n"
	    "Load: %s\n",
	    max_worker_cnt, min_worker_cnt,
	    max_concurrent_workers, max_items_per_worker,
	    total,
	    active,
	    blocked,
	    idle,
	    items,
	    stopping,
	    load_str);
}

/** Prints stats of the global work queue. */
void workq_global_print_info(void)
{
	workq_print_info(&g_work_queue);
}


static bool dequeue_add_req(nonblock_adder_t *info, struct work_queue **pworkq)
{
	bool stop = false;

	irq_spinlock_lock(&info->lock, true);

	while (list_empty(&info->work_queues) && !stop) {
		errno_t ret = _condvar_wait_timeout_irq_spinlock(&info->req_cv,
		    &info->lock, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_INTERRUPTIBLE);

		stop = (ret == EINTR);
	}

	if (!stop) {
		*pworkq = list_get_instance(list_first(&info->work_queues),
		    struct work_queue, nb_link);

		assert(!workq_corrupted(*pworkq));

		list_remove(&(*pworkq)->nb_link);
	}

	irq_spinlock_unlock(&info->lock, true);

	return !stop;
}

static void thr_nonblock_add_worker(void *arg)
{
	nonblock_adder_t *info = arg;
	struct work_queue *workq = NULL;

	while (dequeue_add_req(info, &workq)) {
		add_worker(workq);
	}
}


static void nonblock_init(void)
{
	irq_spinlock_initialize(&nonblock_adder.lock, "kworkq-nb.lock");
	condvar_initialize(&nonblock_adder.req_cv);
	list_initialize(&nonblock_adder.work_queues);

	nonblock_adder.thread = thread_create(thr_nonblock_add_worker,
	    &nonblock_adder, TASK, THREAD_FLAG_NONE, "kworkq-nb");

	if (nonblock_adder.thread) {
		thread_ready(nonblock_adder.thread);
	} else {
		/*
		 * We won't be able to add workers without blocking if all workers
		 * sleep, but at least boot the system.
		 */
		printf("Failed to create kworkq-nb. Sleeping work may stall the workq.\n");
	}
}

#ifdef CONFIG_DEBUG
/** Returns true if the workq is definitely corrupted; false if not sure.
 *
 * Can be used outside of any locks.
 */
static bool workq_corrupted(struct work_queue *workq)
{
	/*
	 * Needed to make the most current cookie value set by workq_preinit()
	 * visible even if we access the workq right after it is created but
	 * on a different cpu. Otherwise, workq_corrupted() would not work
	 * outside a lock.
	 */
	memory_barrier();
	return NULL == workq || workq->cookie != WORKQ_MAGIC;
}

/** Returns true if the work_item is definitely corrupted; false if not sure.
 *
 * Must be used with the work queue protecting spinlock locked.
 */
static bool work_item_corrupted(work_t *work_item)
{
	return NULL == work_item || work_item->cookie != WORK_ITEM_MAGIC;
}
#endif

/** @}
 */
