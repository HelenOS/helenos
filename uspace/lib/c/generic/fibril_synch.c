/*
 * Copyright (c) 2009 Jakub Jermar 
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <fibril_synch.h>
#include <fibril.h>
#include <async.h>
#include <adt/list.h>
#include <futex.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include <stacktrace.h>
#include <stdlib.h>
#include <stdio.h>
#include "private/async.h"

static void optimize_execution_power(void)
{
	/*
	 * When waking up a worker fibril previously blocked in fibril
	 * synchronization, chances are that there is an idle manager fibril
	 * waiting for IPC, that could start executing the awakened worker
	 * fibril right away. We try to detect this and bring the manager
	 * fibril back to fruitful work.
	 */
	if (atomic_get(&threads_in_ipc_wait) > 0)
		async_poke();
}

static void print_deadlock(fibril_owner_info_t *oi)
{
	fibril_t *f = (fibril_t *) fibril_get_id();

	printf("Deadlock detected.\n");
	stacktrace_print();

	printf("Fibril %p waits for primitive %p.\n", f, oi);

	while (oi && oi->owned_by) {
		printf("Primitive %p is owned by fibril %p.\n",
		    oi, oi->owned_by);
		if (oi->owned_by == f)
			break;
		stacktrace_print_fp_pc(context_get_fp(&oi->owned_by->ctx),
		    oi->owned_by->ctx.pc);
		printf("Fibril %p waits for primitive %p.\n",
		     oi->owned_by, oi->owned_by->waits_for);
		oi = oi->owned_by->waits_for;
	}
}


static void check_for_deadlock(fibril_owner_info_t *oi)
{
	while (oi && oi->owned_by) {
		if (oi->owned_by == (fibril_t *) fibril_get_id()) {
			print_deadlock(oi);
			abort();
		}
		oi = oi->owned_by->waits_for;
	}
}


void fibril_mutex_initialize(fibril_mutex_t *fm)
{
	fm->oi.owned_by = NULL;
	fm->counter = 1;
	list_initialize(&fm->waiters);
}

void fibril_mutex_lock(fibril_mutex_t *fm)
{
	fibril_t *f = (fibril_t *) fibril_get_id();

	futex_down(&async_futex);
	if (fm->counter-- <= 0) {
		awaiter_t wdata;

		awaiter_initialize(&wdata);
		wdata.fid = fibril_get_id();
		wdata.wu_event.inlist = true;
		list_append(&wdata.wu_event.link, &fm->waiters);
		check_for_deadlock(&fm->oi);
		f->waits_for = &fm->oi;
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		fm->oi.owned_by = f;
		futex_up(&async_futex);
	}
}

bool fibril_mutex_trylock(fibril_mutex_t *fm)
{
	bool locked = false;
	
	futex_down(&async_futex);
	if (fm->counter > 0) {
		fm->counter--;
		fm->oi.owned_by = (fibril_t *) fibril_get_id();
		locked = true;
	}
	futex_up(&async_futex);
	
	return locked;
}

static void _fibril_mutex_unlock_unsafe(fibril_mutex_t *fm)
{
	if (fm->counter++ < 0) {
		link_t *tmp;
		awaiter_t *wdp;
		fibril_t *f;
	
		tmp = list_first(&fm->waiters);
		assert(tmp != NULL);
		wdp = list_get_instance(tmp, awaiter_t, wu_event.link);
		wdp->active = true;
		wdp->wu_event.inlist = false;

		f = (fibril_t *) wdp->fid;
		fm->oi.owned_by = f;
		f->waits_for = NULL;

		list_remove(&wdp->wu_event.link);
		fibril_add_ready(wdp->fid);
		optimize_execution_power();
	} else {
		fm->oi.owned_by = NULL;
	}
}

void fibril_mutex_unlock(fibril_mutex_t *fm)
{
	assert(fibril_mutex_is_locked(fm));
	futex_down(&async_futex);
	_fibril_mutex_unlock_unsafe(fm);
	futex_up(&async_futex);
}

bool fibril_mutex_is_locked(fibril_mutex_t *fm)
{
	bool locked = false;
	
	futex_down(&async_futex);
	if (fm->counter <= 0) 
		locked = true;
	futex_up(&async_futex);
	
	return locked;
}

void fibril_rwlock_initialize(fibril_rwlock_t *frw)
{
	frw->oi.owned_by = NULL;
	frw->writers = 0;
	frw->readers = 0;
	list_initialize(&frw->waiters);
}

void fibril_rwlock_read_lock(fibril_rwlock_t *frw)
{
	fibril_t *f = (fibril_t *) fibril_get_id();
	
	futex_down(&async_futex);
	if (frw->writers) {
		awaiter_t wdata;

		awaiter_initialize(&wdata);
		wdata.fid = (fid_t) f;
		wdata.wu_event.inlist = true;
		f->flags &= ~FIBRIL_WRITER;
		list_append(&wdata.wu_event.link, &frw->waiters);
		check_for_deadlock(&frw->oi);
		f->waits_for = &frw->oi;
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		/* Consider the first reader the owner. */
		if (frw->readers++ == 0)
			frw->oi.owned_by = f;
		futex_up(&async_futex);
	}
}

void fibril_rwlock_write_lock(fibril_rwlock_t *frw)
{
	fibril_t *f = (fibril_t *) fibril_get_id();
	
	futex_down(&async_futex);
	if (frw->writers || frw->readers) {
		awaiter_t wdata;

		awaiter_initialize(&wdata);
		wdata.fid = (fid_t) f;
		wdata.wu_event.inlist = true;
		f->flags |= FIBRIL_WRITER;
		list_append(&wdata.wu_event.link, &frw->waiters);
		check_for_deadlock(&frw->oi);
		f->waits_for = &frw->oi;
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		frw->oi.owned_by = f;
		frw->writers++;
		futex_up(&async_futex);
	}
}

static void _fibril_rwlock_common_unlock(fibril_rwlock_t *frw)
{
	futex_down(&async_futex);
	if (frw->readers) {
		if (--frw->readers) {
			if (frw->oi.owned_by == (fibril_t *) fibril_get_id()) {
				/*
				 * If this reader firbril was considered the
				 * owner of this rwlock, clear the ownership
				 * information even if there are still more
				 * readers.
				 *
				 * This is the limitation of the detection
				 * mechanism rooted in the fact that tracking
				 * all readers would require dynamically
				 * allocated memory for keeping linkage info.
				 */
				frw->oi.owned_by = NULL;
			}
			goto out;
		}
	} else {
		frw->writers--;
	}
	
	assert(!frw->readers && !frw->writers);
	
	frw->oi.owned_by = NULL;
	
	while (!list_empty(&frw->waiters)) {
		link_t *tmp = list_first(&frw->waiters);
		awaiter_t *wdp;
		fibril_t *f;
		
		wdp = list_get_instance(tmp, awaiter_t, wu_event.link);
		f = (fibril_t *) wdp->fid;
		
		f->waits_for = NULL;
		
		if (f->flags & FIBRIL_WRITER) {
			if (frw->readers)
				break;
			wdp->active = true;
			wdp->wu_event.inlist = false;
			list_remove(&wdp->wu_event.link);
			fibril_add_ready(wdp->fid);
			frw->writers++;
			frw->oi.owned_by = f;
			optimize_execution_power();
			break;
		} else {
			wdp->active = true;
			wdp->wu_event.inlist = false;
			list_remove(&wdp->wu_event.link);
			fibril_add_ready(wdp->fid);
			if (frw->readers++ == 0) {
				/* Consider the first reader the owner. */
				frw->oi.owned_by = f;
			}
			optimize_execution_power();
		}
	}
out:
	futex_up(&async_futex);
}

void fibril_rwlock_read_unlock(fibril_rwlock_t *frw)
{
	assert(fibril_rwlock_is_read_locked(frw));
	_fibril_rwlock_common_unlock(frw);
}

void fibril_rwlock_write_unlock(fibril_rwlock_t *frw)
{
	assert(fibril_rwlock_is_write_locked(frw));
	_fibril_rwlock_common_unlock(frw);
}

bool fibril_rwlock_is_read_locked(fibril_rwlock_t *frw)
{
	bool locked = false;

	futex_down(&async_futex);
	if (frw->readers)
		locked = true;
	futex_up(&async_futex);

	return locked;
}

bool fibril_rwlock_is_write_locked(fibril_rwlock_t *frw)
{
	bool locked = false;

	futex_down(&async_futex);
	if (frw->writers) {
		assert(frw->writers == 1);
		locked = true;
	}
	futex_up(&async_futex);

	return locked;
}

bool fibril_rwlock_is_locked(fibril_rwlock_t *frw)
{
	return fibril_rwlock_is_read_locked(frw) ||
	    fibril_rwlock_is_write_locked(frw);
}

void fibril_condvar_initialize(fibril_condvar_t *fcv)
{
	list_initialize(&fcv->waiters);
}

errno_t
fibril_condvar_wait_timeout(fibril_condvar_t *fcv, fibril_mutex_t *fm,
    suseconds_t timeout)
{
	awaiter_t wdata;

	assert(fibril_mutex_is_locked(fm));

	if (timeout < 0)
		return ETIMEOUT;

	awaiter_initialize(&wdata);
	wdata.fid = fibril_get_id();
	wdata.to_event.inlist = timeout > 0;
	wdata.wu_event.inlist = true;

	futex_down(&async_futex);
	if (timeout) {
		getuptime(&wdata.to_event.expires);
		tv_add_diff(&wdata.to_event.expires, timeout);
		async_insert_timeout(&wdata);
	}
	list_append(&wdata.wu_event.link, &fcv->waiters);
	_fibril_mutex_unlock_unsafe(fm);
	fibril_switch(FIBRIL_TO_MANAGER);
	fibril_mutex_lock(fm);

	/* async_futex not held after fibril_switch() */
	futex_down(&async_futex);
	if (wdata.to_event.inlist)
		list_remove(&wdata.to_event.link);
	if (wdata.wu_event.inlist)
		list_remove(&wdata.wu_event.link);
	futex_up(&async_futex);
	
	return wdata.to_event.occurred ? ETIMEOUT : EOK;
}

void fibril_condvar_wait(fibril_condvar_t *fcv, fibril_mutex_t *fm)
{
	errno_t rc;

	rc = fibril_condvar_wait_timeout(fcv, fm, 0);
	assert(rc == EOK);
}

static void _fibril_condvar_wakeup_common(fibril_condvar_t *fcv, bool once)
{
	link_t *tmp;
	awaiter_t *wdp;

	futex_down(&async_futex);
	while (!list_empty(&fcv->waiters)) {
		tmp = list_first(&fcv->waiters);
		wdp = list_get_instance(tmp, awaiter_t, wu_event.link);
		list_remove(&wdp->wu_event.link);
		wdp->wu_event.inlist = false;
		if (!wdp->active) {
			wdp->active = true;
			fibril_add_ready(wdp->fid);
			optimize_execution_power();
			if (once)
				break;
		}
	}
	futex_up(&async_futex);
}

void fibril_condvar_signal(fibril_condvar_t *fcv)
{
	_fibril_condvar_wakeup_common(fcv, true);
}

void fibril_condvar_broadcast(fibril_condvar_t *fcv)
{
	_fibril_condvar_wakeup_common(fcv, false);
}

/** Timer fibril.
 *
 * @param arg	Timer
 */
static errno_t fibril_timer_func(void *arg)
{
	fibril_timer_t *timer = (fibril_timer_t *) arg;
	errno_t rc;

	fibril_mutex_lock(timer->lockp);

	while (timer->state != fts_cleanup) {
		switch (timer->state) {
		case fts_not_set:
		case fts_fired:
			fibril_condvar_wait(&timer->cv, timer->lockp);
			break;
		case fts_active:
			rc = fibril_condvar_wait_timeout(&timer->cv,
			    timer->lockp, timer->delay);
			if (rc == ETIMEOUT && timer->state == fts_active) {
				timer->state = fts_fired;
				timer->handler_fid = fibril_get_id();
				fibril_mutex_unlock(timer->lockp);
				timer->fun(timer->arg);
				fibril_mutex_lock(timer->lockp);
				timer->handler_fid = 0;
			}
			break;
		case fts_cleanup:
		case fts_clean:
			assert(false);
			break;
		}
	}

	/* Acknowledge timer fibril has finished cleanup. */
	timer->state = fts_clean;
	fibril_condvar_broadcast(&timer->cv);
	fibril_mutex_unlock(timer->lockp);

	return 0;
}

/** Create new timer.
 *
 * @return		New timer on success, @c NULL if out of memory.
 */
fibril_timer_t *fibril_timer_create(fibril_mutex_t *lock)
{
	fid_t fid;
	fibril_timer_t *timer;

	timer = calloc(1, sizeof(fibril_timer_t));
	if (timer == NULL)
		return NULL;

	fid = fibril_create(fibril_timer_func, (void *) timer);
	if (fid == 0) {
		free(timer);
		return NULL;
	}

	fibril_mutex_initialize(&timer->lock);
	fibril_condvar_initialize(&timer->cv);

	timer->fibril = fid;
	timer->state = fts_not_set;
	timer->lockp = (lock != NULL) ? lock : &timer->lock;

	fibril_add_ready(fid);
	return timer;
}

/** Destroy timer.
 *
 * @param timer		Timer, must not be active or accessed by other threads.
 */
void fibril_timer_destroy(fibril_timer_t *timer)
{
	fibril_mutex_lock(timer->lockp);
	assert(timer->state == fts_not_set || timer->state == fts_fired);

	/* Request timer fibril to terminate. */
	timer->state = fts_cleanup;
	fibril_condvar_broadcast(&timer->cv);

	/* Wait for timer fibril to terminate */
	while (timer->state != fts_clean)
		fibril_condvar_wait(&timer->cv, timer->lockp);
	fibril_mutex_unlock(timer->lockp);

	free(timer);
}

/** Set timer.
 *
 * Set timer to execute a callback function after the specified
 * interval.
 *
 * @param timer		Timer
 * @param delay		Delay in microseconds
 * @param fun		Callback function
 * @param arg		Argument for @a fun
 */
void fibril_timer_set(fibril_timer_t *timer, suseconds_t delay,
    fibril_timer_fun_t fun, void *arg)
{
	fibril_mutex_lock(timer->lockp);
	fibril_timer_set_locked(timer, delay, fun, arg);
	fibril_mutex_unlock(timer->lockp);
}

/** Set locked timer.
 *
 * Set timer to execute a callback function after the specified
 * interval. Must be called when the timer is locked.
 *
 * @param timer		Timer
 * @param delay		Delay in microseconds
 * @param fun		Callback function
 * @param arg		Argument for @a fun
 */
void fibril_timer_set_locked(fibril_timer_t *timer, suseconds_t delay,
    fibril_timer_fun_t fun, void *arg)
{
	assert(fibril_mutex_is_locked(timer->lockp));
	assert(timer->state == fts_not_set || timer->state == fts_fired);
	timer->state = fts_active;
	timer->delay = delay;
	timer->fun = fun;
	timer->arg = arg;
	fibril_condvar_broadcast(&timer->cv);
}

/** Clear timer.
 *
 * Clears (cancels) timer and returns last state of the timer.
 * This can be one of:
 *    - fts_not_set	If the timer has not been set or has been cleared
 *    - fts_active	Timer was set but did not fire
 *    - fts_fired	Timer fired
 *
 * @param timer		Timer
 * @return		Last timer state
 */
fibril_timer_state_t fibril_timer_clear(fibril_timer_t *timer)
{
	fibril_timer_state_t old_state;

	fibril_mutex_lock(timer->lockp);
	old_state = fibril_timer_clear_locked(timer);
	fibril_mutex_unlock(timer->lockp);

	return old_state;
}

/** Clear locked timer.
 *
 * Clears (cancels) timer and returns last state of the timer.
 * This can be one of:
 *    - fts_not_set	If the timer has not been set or has been cleared
 *    - fts_active	Timer was set but did not fire
 *    - fts_fired	Timer fired
 * Must be called when the timer is locked.
 *
 * @param timer		Timer
 * @return		Last timer state
 */
fibril_timer_state_t fibril_timer_clear_locked(fibril_timer_t *timer)
{
	fibril_timer_state_t old_state;

	assert(fibril_mutex_is_locked(timer->lockp));

	while (timer->handler_fid != 0) {
		if (timer->handler_fid == fibril_get_id()) {
			printf("Deadlock detected.\n");
			stacktrace_print();
			printf("Fibril %zx is trying to clear timer %p from "
			    "inside its handler %p.\n",
			    fibril_get_id(), timer, timer->fun);
			abort();
		}

		fibril_condvar_wait(&timer->cv, timer->lockp);
	}

	old_state = timer->state;
	timer->state = fts_not_set;

	timer->delay = 0;
	timer->fun = NULL;
	timer->arg = NULL;
	fibril_condvar_broadcast(&timer->cv);

	return old_state;
}

/** @}
 */
