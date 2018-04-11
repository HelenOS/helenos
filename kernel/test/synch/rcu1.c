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

#include <assert.h>
#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <print.h>
#include <proc/thread.h>
#include <macros.h>
#include <str.h>
#include <errno.h>
#include <time/delay.h>

#include <synch/rcu.h>


#define MAX_THREADS 32

static int one_idx = 0;
static thread_t *thread[MAX_THREADS] = { NULL };

typedef struct {
	rcu_item_t rcu;
	bool exited;
} exited_t;

/* Co-opt EPARTY error code for race detection. */
#define ERACE   EPARTY

/*-------------------------------------------------------------------*/
static void wait_for_cb_exit(size_t secs, exited_t *p, errno_t *presult)
{
	size_t loops = 0;
	/* 4 secs max */
	size_t loop_ms_sec = 500;
	size_t max_loops = ((secs * 1000 + loop_ms_sec - 1) / loop_ms_sec);

	while (loops < max_loops && !p->exited) {
		++loops;
		thread_usleep(loop_ms_sec * 1000);
		TPRINTF(".");
	}

	if (!p->exited) {
		*presult = ETIMEOUT;
	}
}

static size_t get_thread_cnt(void)
{
	return min(MAX_THREADS, config.cpu_active * 4);
}

static void run_thread(size_t k, void (*func)(void *), void *arg)
{
	assert(thread[k] == NULL);

	thread[k] = thread_create(func, arg, TASK, THREAD_FLAG_NONE,
	    "test-rcu-thread");

	if (thread[k]) {
		/* Distribute evenly. */
		thread_wire(thread[k], &cpus[k % config.cpu_active]);
		thread_ready(thread[k]);
	}
}

static void run_all(void (*func)(void *))
{
	size_t thread_cnt = get_thread_cnt();

	one_idx = 0;

	for (size_t i = 0; i < thread_cnt; ++i) {
		run_thread(i, func, NULL);
	}
}

static void join_all(void)
{
	size_t thread_cnt = get_thread_cnt();

	one_idx = 0;

	for (size_t i = 0; i < thread_cnt; ++i) {
		if (thread[i]) {
			bool joined = false;
			do {
				errno_t ret = thread_join_timeout(thread[i], 5 * 1000 * 1000, 0);
				joined = (ret != ETIMEOUT);

				if (ret == EOK) {
					TPRINTF("%zu threads remain\n", thread_cnt - i - 1);
				}
			} while (!joined);

			thread_detach(thread[i]);
			thread[i] = NULL;
		}
	}
}

static void run_one(void (*func)(void *), void *arg)
{
	assert(one_idx < MAX_THREADS);
	run_thread(one_idx, func, arg);
	++one_idx;
}


static void join_one(void)
{
	assert(0 < one_idx && one_idx <= MAX_THREADS);

	--one_idx;

	if (thread[one_idx]) {
		thread_join(thread[one_idx]);
		thread_detach(thread[one_idx]);
		thread[one_idx] = NULL;
	}
}

/*-------------------------------------------------------------------*/


static void nop_reader(void *arg)
{
	size_t nop_iters = (size_t)arg;

	TPRINTF("Enter nop-reader\n");

	for (size_t i = 0; i < nop_iters; ++i) {
		rcu_read_lock();
		rcu_read_unlock();
	}

	TPRINTF("Exit nop-reader\n");
}

static void get_seq(size_t from, size_t to, size_t steps, size_t *seq)
{
	assert(0 < steps && from <= to && 0 < to);
	size_t inc = (to - from) / (steps - 1);

	for (size_t i = 0; i < steps - 1; ++i) {
		seq[i] = i * inc + from;
	}

	seq[steps - 1] = to;
}

static bool do_nop_readers(void)
{
	size_t seq[MAX_THREADS] = { 0 };
	get_seq(100, 100000, get_thread_cnt(), seq);

	TPRINTF("\nRun %zu thr: repeat empty no-op reader sections\n", get_thread_cnt());

	for (size_t k = 0; k < get_thread_cnt(); ++k)
		run_one(nop_reader, (void *)seq[k]);

	TPRINTF("\nJoining %zu no-op readers\n", get_thread_cnt());
	join_all();

	return true;
}

/*-------------------------------------------------------------------*/



static void long_reader(void *arg)
{
	const size_t iter_cnt = 100 * 1000 * 1000;
	size_t nop_iters = (size_t)arg;
	size_t outer_iters = iter_cnt / nop_iters;

	TPRINTF("Enter long-reader\n");

	for (size_t i = 0; i < outer_iters; ++i) {
		rcu_read_lock();

		for (volatile size_t k = 0; k < nop_iters; ++k) {
			/* nop, but increment volatile k */
		}

		rcu_read_unlock();
	}

	TPRINTF("Exit long-reader\n");
}

static bool do_long_readers(void)
{
	size_t seq[MAX_THREADS] = { 0 };
	get_seq(10, 1000 * 1000, get_thread_cnt(), seq);

	TPRINTF("\nRun %zu thr: repeat long reader sections, will preempt, no cbs.\n",
	    get_thread_cnt());

	for (size_t k = 0; k < get_thread_cnt(); ++k)
		run_one(long_reader, (void *)seq[k]);

	TPRINTF("\nJoining %zu readers with long reader sections.\n", get_thread_cnt());
	join_all();

	return true;
}

/*-------------------------------------------------------------------*/


static atomic_t nop_callbacks_cnt = { 0 };
/* Must be even. */
static const int nop_updater_iters = 10000;

static void count_cb(rcu_item_t *item)
{
	atomic_inc(&nop_callbacks_cnt);
	free(item);
}

static void nop_updater(void *arg)
{
	for (int i = 0; i < nop_updater_iters; i += 2) {
		rcu_item_t *a = malloc(sizeof(rcu_item_t), FRAME_ATOMIC);
		rcu_item_t *b = malloc(sizeof(rcu_item_t), FRAME_ATOMIC);

		if (a && b) {
			rcu_call(a, count_cb);
			rcu_call(b, count_cb);
		} else {
			TPRINTF("[out-of-mem]\n");
			free(a);
			free(b);
			return;
		}
	}
}

static bool do_nop_callbacks(void)
{
	atomic_set(&nop_callbacks_cnt, 0);

	size_t exp_cnt = nop_updater_iters * get_thread_cnt();
	size_t max_used_mem = sizeof(rcu_item_t) * exp_cnt;

	TPRINTF("\nRun %zu thr: post %zu no-op callbacks (%zu B used), no readers.\n",
	    get_thread_cnt(), exp_cnt, max_used_mem);

	run_all(nop_updater);
	TPRINTF("\nJoining %zu no-op callback threads\n", get_thread_cnt());
	join_all();

	size_t loop_cnt = 0, max_loops = 15;

	while (exp_cnt != atomic_get(&nop_callbacks_cnt) && loop_cnt < max_loops) {
		++loop_cnt;
		TPRINTF(".");
		thread_sleep(1);
	}

	return loop_cnt < max_loops;
}

/*-------------------------------------------------------------------*/

typedef struct {
	rcu_item_t rcu_item;
	int cookie;
} item_w_cookie_t;

const int magic_cookie = 0x01234567;
static int one_cb_is_done = 0;

static void one_cb_done(rcu_item_t *item)
{
	assert(((item_w_cookie_t *)item)->cookie == magic_cookie);
	one_cb_is_done = 1;
	TPRINTF("Callback()\n");
	free(item);
}

static void one_cb_reader(void *arg)
{
	TPRINTF("Enter one-cb-reader\n");

	rcu_read_lock();

	item_w_cookie_t *item = malloc(sizeof(item_w_cookie_t), FRAME_ATOMIC);

	if (item) {
		item->cookie = magic_cookie;
		rcu_call(&item->rcu_item, one_cb_done);
	} else {
		TPRINTF("\n[out-of-mem]\n");
	}

	thread_sleep(1);

	rcu_read_unlock();

	TPRINTF("Exit one-cb-reader\n");
}

static bool do_one_cb(void)
{
	one_cb_is_done = 0;

	TPRINTF("\nRun a single reader that posts one callback.\n");
	run_one(one_cb_reader, NULL);
	join_one();

	TPRINTF("\nJoined one-cb reader, wait for callback.\n");
	size_t loop_cnt = 0;
	size_t max_loops = 4; /* 200 ms total */

	while (!one_cb_is_done && loop_cnt < max_loops) {
		thread_usleep(50 * 1000);
		++loop_cnt;
	}

	return one_cb_is_done;
}

/*-------------------------------------------------------------------*/

typedef struct {
	size_t update_cnt;
	size_t read_cnt;
	size_t iters;
} seq_work_t;

typedef struct {
	rcu_item_t rcu;
	atomic_count_t start_time;
} seq_item_t;


static errno_t seq_test_result = EOK;

static atomic_t cur_time = { 1 };
static atomic_count_t max_upd_done_time = { 0 };

static void seq_cb(rcu_item_t *rcu_item)
{
	seq_item_t *item = member_to_inst(rcu_item, seq_item_t, rcu);

	/* Racy but errs to the conservative side, so it is ok. */
	if (max_upd_done_time < item->start_time) {
		max_upd_done_time = item->start_time;

		/* Make updated time visible */
		memory_barrier();
	}

	free(item);
}

static void seq_func(void *arg)
{
	/*
	 * Temporarily workaround GCC 7.1.0 internal
	 * compiler error when compiling for riscv64.
	 */
#ifndef KARCH_riscv64
	seq_work_t *work = (seq_work_t *)arg;

	/* Alternate between reader and updater roles. */
	for (size_t k = 0; k < work->iters; ++k) {
		/* Reader */
		for (size_t i = 0; i < work->read_cnt; ++i) {
			rcu_read_lock();
			atomic_count_t start_time = atomic_postinc(&cur_time);

			for (volatile size_t d = 0; d < 10 * i; ++d) {
				/* no-op */
			}

			/* Get most recent max_upd_done_time. */
			memory_barrier();

			if (start_time < max_upd_done_time) {
				seq_test_result = ERACE;
			}

			rcu_read_unlock();

			if (seq_test_result != EOK)
				return;
		}

		/* Updater */
		for (size_t i = 0; i < work->update_cnt; ++i) {
			seq_item_t *a = malloc(sizeof(seq_item_t), FRAME_ATOMIC);
			seq_item_t *b = malloc(sizeof(seq_item_t), FRAME_ATOMIC);

			if (a && b) {
				a->start_time = atomic_postinc(&cur_time);
				rcu_call(&a->rcu, seq_cb);

				b->start_time = atomic_postinc(&cur_time);
				rcu_call(&b->rcu, seq_cb);
			} else {
				TPRINTF("\n[out-of-mem]\n");
				seq_test_result = ENOMEM;
				free(a);
				free(b);
				return;
			}
		}

	}
#else
	(void) seq_cb;
#endif
}

static bool do_seq_check(void)
{
	seq_test_result = EOK;
	max_upd_done_time = 0;
	atomic_set(&cur_time, 1);

	const size_t iters = 100;
	const size_t total_cnt = 1000;
	size_t read_cnt[MAX_THREADS] = { 0 };
	seq_work_t item[MAX_THREADS];

	size_t total_cbs = 0;
	size_t max_used_mem = 0;

	get_seq(0, total_cnt, get_thread_cnt(), read_cnt);


	for (size_t i = 0; i < get_thread_cnt(); ++i) {
		item[i].update_cnt = total_cnt - read_cnt[i];
		item[i].read_cnt = read_cnt[i];
		item[i].iters = iters;

		total_cbs += 2 * iters * item[i].update_cnt;
	}

	max_used_mem = total_cbs * sizeof(seq_item_t);

	const char *mem_suffix;
	uint64_t mem_units;
	bin_order_suffix(max_used_mem, &mem_units, &mem_suffix, false);

	TPRINTF("\nRun %zu th: check callback completion time in readers. "
	    "%zu callbacks total (max %" PRIu64 " %s used). Be patient.\n",
	    get_thread_cnt(), total_cbs, mem_units, mem_suffix);

	for (size_t i = 0; i < get_thread_cnt(); ++i) {
		run_one(seq_func, &item[i]);
	}

	TPRINTF("\nJoining %zu seq-threads\n", get_thread_cnt());
	join_all();

	if (seq_test_result == ENOMEM) {
		TPRINTF("\nErr: out-of mem\n");
	} else if (seq_test_result == ERACE) {
		TPRINTF("\nERROR: race detected!!\n");
	}

	return seq_test_result == EOK;
}

/*-------------------------------------------------------------------*/


static void reader_unlocked(rcu_item_t *item)
{
	exited_t *p = (exited_t *)item;
	p->exited = true;
}

static void reader_exit(void *arg)
{
	rcu_read_lock();
	rcu_read_lock();
	rcu_read_lock();
	rcu_read_unlock();

	rcu_call((rcu_item_t *)arg, reader_unlocked);

	rcu_read_lock();
	rcu_read_lock();

	/* Exit without unlocking the rcu reader section. */
}

static bool do_reader_exit(void)
{
	TPRINTF("\nReader exits thread with rcu_lock\n");

	exited_t *p = malloc(sizeof(exited_t), FRAME_ATOMIC);
	if (!p) {
		TPRINTF("[out-of-mem]\n");
		return false;
	}

	p->exited = false;

	run_one(reader_exit, p);
	join_one();

	errno_t result = EOK;
	wait_for_cb_exit(2 /* secs */, p, &result);

	if (result != EOK) {
		TPRINTF("Err: RCU locked up after exiting from within a reader\n");
		/* Leak the mem. */
	} else {
		free(p);
	}

	return result == EOK;
}

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/

typedef struct preempt_struct {
	exited_t e;
	errno_t result;
} preempt_t;


static void preempted_unlocked(rcu_item_t *item)
{
	preempt_t *p = member_to_inst(item, preempt_t, e.rcu);
	p->e.exited = true;
	TPRINTF("Callback().\n");
}

static void preempted_reader_prev(void *arg)
{
	preempt_t *p = (preempt_t *)arg;
	assert(!p->e.exited);

	TPRINTF("reader_prev{ ");

	rcu_read_lock();
	scheduler();
	rcu_read_unlock();

	/*
	 * Start GP after exiting reader section w/ preemption.
	 * Just check that the callback does not lock up and is not lost.
	 */
	rcu_call(&p->e.rcu, preempted_unlocked);

	TPRINTF("}reader_prev\n");
}

static void preempted_reader_inside_cur(void *arg)
{
	preempt_t *p = (preempt_t *)arg;
	assert(!p->e.exited);

	TPRINTF("reader_inside_cur{ ");
	/*
	 * Start a GP and try to finish the reader before
	 * the GP ends (including preemption).
	 */
	rcu_call(&p->e.rcu, preempted_unlocked);

	/* Give RCU threads a chance to start up. */
	scheduler();
	scheduler();

	rcu_read_lock();
	/* Come back as soon as possible to complete before GP ends. */
	thread_usleep(2);
	rcu_read_unlock();

	TPRINTF("}reader_inside_cur\n");
}


static void preempted_reader_cur(void *arg)
{
	preempt_t *p = (preempt_t *)arg;
	assert(!p->e.exited);

	TPRINTF("reader_cur{ ");
	rcu_read_lock();

	/* Start GP. */
	rcu_call(&p->e.rcu, preempted_unlocked);

	/* Preempt while cur GP detection is running */
	thread_sleep(1);

	/* Err: exited before this reader completed. */
	if (p->e.exited)
		p->result = ERACE;

	rcu_read_unlock();
	TPRINTF("}reader_cur\n");
}

static void preempted_reader_next1(void *arg)
{
	preempt_t *p = (preempt_t *)arg;
	assert(!p->e.exited);

	TPRINTF("reader_next1{ ");
	rcu_read_lock();

	/* Preempt before cur GP detection starts. */
	scheduler();

	/* Start GP. */
	rcu_call(&p->e.rcu, preempted_unlocked);

	/* Err: exited before this reader completed. */
	if (p->e.exited)
		p->result = ERACE;

	rcu_read_unlock();
	TPRINTF("}reader_next1\n");
}

static void preempted_reader_next2(void *arg)
{
	preempt_t *p = (preempt_t *)arg;
	assert(!p->e.exited);

	TPRINTF("reader_next2{ ");
	rcu_read_lock();

	/* Preempt before cur GP detection starts. */
	scheduler();

	/* Start GP. */
	rcu_call(&p->e.rcu, preempted_unlocked);

	/*
	 * Preempt twice while GP is running after we've been known
	 * to hold up the GP just to make sure multiple preemptions
	 * are properly tracked if a reader is delaying the cur GP.
	 */
	thread_sleep(1);
	thread_sleep(1);

	/* Err: exited before this reader completed. */
	if (p->e.exited)
		p->result = ERACE;

	rcu_read_unlock();
	TPRINTF("}reader_next2\n");
}


static bool do_one_reader_preempt(void (*f)(void *), const char *err)
{
	preempt_t *p = malloc(sizeof(preempt_t), FRAME_ATOMIC);
	if (!p) {
		TPRINTF("[out-of-mem]\n");
		return false;
	}

	p->e.exited = false;
	p->result = EOK;

	run_one(f, p);
	join_one();

	/* Wait at most 4 secs. */
	wait_for_cb_exit(4, &p->e, &p->result);

	if (p->result == EOK) {
		free(p);
		return true;
	} else {
		TPRINTF("%s", err);
		/* Leak a bit of mem. */
		return false;
	}
}

static bool do_reader_preempt(void)
{
	TPRINTF("\nReaders will be preempted.\n");

	bool success = true;
	bool ok = true;

	ok = do_one_reader_preempt(preempted_reader_prev,
	    "Err: preempted_reader_prev()\n");
	success = success && ok;

	ok = do_one_reader_preempt(preempted_reader_inside_cur,
	    "Err: preempted_reader_inside_cur()\n");
	success = success && ok;

	ok = do_one_reader_preempt(preempted_reader_cur,
	    "Err: preempted_reader_cur()\n");
	success = success && ok;

	ok = do_one_reader_preempt(preempted_reader_next1,
	    "Err: preempted_reader_next1()\n");
	success = success && ok;

	ok = do_one_reader_preempt(preempted_reader_next2,
	    "Err: preempted_reader_next2()\n");
	success = success && ok;

	return success;
}

/*-------------------------------------------------------------------*/
typedef struct {
	bool reader_done;
	bool reader_running;
	bool synch_running;
} synch_t;

static void synch_reader(void *arg)
{
	synch_t *synch = (synch_t *) arg;

	rcu_read_lock();

	/* Order accesses of synch after the reader section begins. */
	memory_barrier();

	synch->reader_running = true;

	while (!synch->synch_running) {
		/* 0.5 sec */
		delay(500 * 1000);
	}

	/* Run for 1 sec */
	delay(1000 * 1000);
	/* thread_join() propagates done to do_synch() */
	synch->reader_done = true;

	rcu_read_unlock();
}


static bool do_synch(void)
{
	TPRINTF("\nSynchronize with long reader\n");

	synch_t *synch = malloc(sizeof(synch_t), FRAME_ATOMIC);

	if (!synch) {
		TPRINTF("[out-of-mem]\n");
		return false;
	}

	synch->reader_done = false;
	synch->reader_running = false;
	synch->synch_running = false;

	run_one(synch_reader, synch);

	/* Wait for the reader to enter its critical section. */
	scheduler();
	while (!synch->reader_running) {
		thread_usleep(500 * 1000);
	}

	synch->synch_running = true;

	rcu_synchronize();
	join_one();


	if (synch->reader_done) {
		free(synch);
		return true;
	} else {
		TPRINTF("Err: synchronize() exited prematurely \n");
		/* Leak some mem. */
		return false;
	}
}

/*-------------------------------------------------------------------*/
typedef struct {
	rcu_item_t rcu_item;
	atomic_t done;
} barrier_t;

static void barrier_callback(rcu_item_t *item)
{
	barrier_t *b = member_to_inst(item, barrier_t, rcu_item);
	atomic_set(&b->done, 1);
}

static bool do_barrier(void)
{
	TPRINTF("\nrcu_barrier: Wait for outstanding rcu callbacks to complete\n");

	barrier_t *barrier = malloc(sizeof(barrier_t), FRAME_ATOMIC);

	if (!barrier) {
		TPRINTF("[out-of-mem]\n");
		return false;
	}

	atomic_set(&barrier->done, 0);

	rcu_call(&barrier->rcu_item, barrier_callback);
	rcu_barrier();

	if (1 == atomic_get(&barrier->done)) {
		free(barrier);
		return true;
	} else {
		TPRINTF("rcu_barrier() exited prematurely.\n");
		/* Leak some mem. */
		return false;
	}
}

/*-------------------------------------------------------------------*/

typedef struct {
	size_t iters;
	bool master;
} stress_t;


static void stress_reader(void *arg)
{
	bool *done = (bool *) arg;

	while (!*done) {
		rcu_read_lock();
		rcu_read_unlock();

		/*
		 * Do some work outside of the reader section so we are not always
		 * preempted in the reader section.
		 */
		delay(5);
	}
}

static void stress_cb(rcu_item_t *item)
{
	/* 5 us * 1000 * 1000 iters == 5 sec per updater thread */
	delay(5);
	free(item);
}

static void stress_updater(void *arg)
{
	stress_t *s = (stress_t *)arg;

	for (size_t i = 0; i < s->iters; ++i) {
		rcu_item_t *item = malloc(sizeof(rcu_item_t), FRAME_ATOMIC);

		if (item) {
			rcu_call(item, stress_cb);
		} else {
			TPRINTF("[out-of-mem]\n");
			return;
		}

		/* Print a dot if we make a progress of 1% */
		if (s->master && 0 == (i % (s->iters / 100)))
			TPRINTF(".");
	}
}

static bool do_stress(void)
{
	size_t cb_per_thread = 1000 * 1000;
	bool done = false;
	stress_t master = { .iters = cb_per_thread, .master = true };
	stress_t worker = { .iters = cb_per_thread, .master = false };

	size_t thread_cnt = min(MAX_THREADS / 2, config.cpu_active);
	/* Each cpu has one reader and one updater. */
	size_t reader_cnt = thread_cnt;
	size_t updater_cnt = thread_cnt;

	size_t exp_upd_calls = updater_cnt * cb_per_thread;
	size_t max_used_mem = exp_upd_calls * sizeof(rcu_item_t);

	const char *mem_suffix;
	uint64_t mem_units;
	bin_order_suffix(max_used_mem, &mem_units, &mem_suffix, false);

	TPRINTF("\nStress: Run %zu nop-readers and %zu updaters. %zu callbacks"
	    " total (max %" PRIu64 " %s used). Be very patient.\n",
	    reader_cnt, updater_cnt, exp_upd_calls, mem_units, mem_suffix);

	for (size_t k = 0; k < reader_cnt; ++k) {
		run_one(stress_reader, &done);
	}

	for (size_t k = 0; k < updater_cnt; ++k) {
		run_one(stress_updater, k > 0 ? &worker : &master);
	}

	TPRINTF("\nJoining %zu stress updaters.\n", updater_cnt);

	for (size_t k = 0; k < updater_cnt; ++k) {
		join_one();
	}

	done = true;

	TPRINTF("\nJoining %zu stress nop-readers.\n", reader_cnt);

	join_all();
	return true;
}
/*-------------------------------------------------------------------*/

typedef struct {
	rcu_item_t r;
	size_t total_cnt;
	size_t count_down;
	bool expedite;
} expedite_t;

static void expedite_cb(rcu_item_t *arg)
{
	expedite_t *e = (expedite_t *)arg;

	if (1 < e->count_down) {
		--e->count_down;

		if (0 == (e->count_down % (e->total_cnt / 100))) {
			TPRINTF("*");
		}

		_rcu_call(e->expedite, &e->r, expedite_cb);
	} else {
		/* Do not touch any of e's mem after we declare we're done with it. */
		memory_barrier();
		e->count_down = 0;
	}
}

static void run_expedite(bool exp, size_t cnt)
{
	expedite_t e;
	e.total_cnt = cnt;
	e.count_down = cnt;
	e.expedite = exp;

	_rcu_call(e.expedite, &e.r, expedite_cb);

	while (0 < e.count_down) {
		thread_sleep(1);
		TPRINTF(".");
	}
}

static bool do_expedite(void)
{
	size_t exp_cnt = 1000 * 1000;
	size_t normal_cnt = 1 * 1000;

	TPRINTF("Expedited: sequence of %zu rcu_calls\n", exp_cnt);
	run_expedite(true, exp_cnt);
	TPRINTF("Normal/non-expedited: sequence of %zu rcu_calls\n", normal_cnt);
	run_expedite(false, normal_cnt);
	return true;
}
/*-------------------------------------------------------------------*/

struct test_func {
	bool include;
	bool (*func)(void);
	const char *desc;
};


const char *test_rcu1(void)
{
	struct test_func test_func[] = {
		{ 1, do_one_cb, "do_one_cb" },
		{ 1, do_reader_preempt, "do_reader_preempt" },
		{ 1, do_synch, "do_synch" },
		{ 1, do_barrier, "do_barrier" },
		{ 1, do_reader_exit, "do_reader_exit" },
		{ 1, do_nop_readers, "do_nop_readers" },
		{ 1, do_seq_check, "do_seq_check" },
		{ 0, do_long_readers, "do_long_readers" },
		{ 1, do_nop_callbacks, "do_nop_callbacks" },
		{ 0, do_expedite, "do_expedite" },
		{ 1, do_stress, "do_stress" },
		{ 0, NULL, NULL }
	};

	bool success = true;
	bool ok = true;
	uint64_t completed_gps = rcu_completed_gps();
	uint64_t delta_gps = 0;

	for (int i = 0; test_func[i].func; ++i) {
		if (!test_func[i].include) {
			TPRINTF("\nSubtest %s() skipped.\n", test_func[i].desc);
			continue;
		} else {
			TPRINTF("\nRunning subtest %s.\n", test_func[i].desc);
		}

		ok = test_func[i].func();
		success = success && ok;

		delta_gps = rcu_completed_gps() - completed_gps;
		completed_gps += delta_gps;

		if (ok) {
			TPRINTF("\nSubtest %s() ok (GPs: %" PRIu64 ").\n",
			    test_func[i].desc, delta_gps);
		} else {
			TPRINTF("\nFailed: %s(). Pausing for 5 secs.\n", test_func[i].desc);
			thread_sleep(5);
		}
	}

	if (success)
		return NULL;
	else
		return "One of the tests failed.";
}
