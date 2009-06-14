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

#include <fibril_sync.h>
#include <fibril.h>
#include <async.h>
#include <adt/list.h>
#include <futex.h>
#include <assert.h>

void fibril_mutex_initialize(fibril_mutex_t *fm)
{
	fm->counter = 1;
	list_initialize(&fm->waiters);
}

void fibril_mutex_lock(fibril_mutex_t *fm)
{
	futex_down(&async_futex);
	if (fm->counter-- <= 0) {
		fibril_t *f = (fibril_t *) fibril_get_id();
		list_append(&f->link, &fm->waiters);
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		futex_up(&async_futex);
	}
}

bool fibril_mutex_trylock(fibril_mutex_t *fm)
{
	bool locked = false;
	
	futex_down(&async_futex);
	if (fm->counter > 0) {
		fm->counter--;
		locked = true;
	}
	futex_up(&async_futex);
	
	return locked;
}

static void _fibril_mutex_unlock_unsafe(fibril_mutex_t *fm)
{
	assert(fm->counter <= 0);
	if (fm->counter++ < 0) {
		link_t *tmp;
		fibril_t *f;
	
		assert(!list_empty(&fm->waiters));
		tmp = fm->waiters.next;
		f = list_get_instance(tmp, fibril_t, link);
		list_remove(&f->link);
		fibril_add_ready((fid_t) f);
	}
}

void fibril_mutex_unlock(fibril_mutex_t *fm)
{
	futex_down(&async_futex);
	_fibril_mutex_unlock_unsafe(fm);
	futex_up(&async_futex);
}

void fibril_rwlock_initialize(fibril_rwlock_t *frw)
{
	frw->writers = 0;
	frw->readers = 0;
	list_initialize(&frw->waiters);
}

void fibril_rwlock_read_lock(fibril_rwlock_t *frw)
{
	futex_down(&async_futex);
	if (frw->writers) {
		fibril_t *f = (fibril_t *) fibril_get_id();
		f->flags &= ~FIBRIL_WRITER;
		list_append(&f->link, &frw->waiters);
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		frw->readers++;
		futex_up(&async_futex);
	}
}

void fibril_rwlock_write_lock(fibril_rwlock_t *frw)
{
	futex_down(&async_futex);
	if (frw->writers || frw->readers) {
		fibril_t *f = (fibril_t *) fibril_get_id();
		f->flags |= FIBRIL_WRITER;
		list_append(&f->link, &frw->waiters);
		fibril_switch(FIBRIL_TO_MANAGER);
	} else {
		frw->writers++;
		futex_up(&async_futex);
	}
}

static void _fibril_rwlock_common_unlock(fibril_rwlock_t *frw)
{
	futex_down(&async_futex);
	assert(frw->readers || (frw->writers == 1));
	if (frw->readers) {
		if (--frw->readers)
			goto out;
	} else {
		frw->writers--;
	}
	
	assert(!frw->readers && !frw->writers);
	
	while (!list_empty(&frw->waiters)) {
		link_t *tmp = frw->waiters.next;
		fibril_t *f = list_get_instance(tmp, fibril_t, link);
		
		if (f->flags & FIBRIL_WRITER) {
			if (frw->readers)
				break;
			list_remove(&f->link);
			fibril_add_ready((fid_t) f);
			frw->writers++;
			break;
		} else {
			list_remove(&f->link);
			fibril_add_ready((fid_t) f);
			frw->readers++;
		}
	}
out:
	futex_up(&async_futex);
}

void fibril_rwlock_read_unlock(fibril_rwlock_t *frw)
{
	_fibril_rwlock_common_unlock(frw);
}

void fibril_rwlock_write_unlock(fibril_rwlock_t *frw)
{
	_fibril_rwlock_common_unlock(frw);
}

void fibril_condvar_initialize(fibril_condvar_t *fcv)
{
	list_initialize(&fcv->waiters);
}

void fibril_condvar_wait(fibril_condvar_t *fcv, fibril_mutex_t *fm)
{
	fibril_t *f = (fibril_t *) fibril_get_id();

	futex_down(&async_futex);
	list_append(&f->link, &fcv->waiters);
	_fibril_mutex_unlock_unsafe(fm);
	fibril_switch(FIBRIL_TO_MANAGER);
	fibril_mutex_lock(fm);
}

static void _fibril_condvar_wakeup_common(fibril_condvar_t *fcv, bool once)
{
	link_t *tmp;
	fibril_t *f;

	futex_down(&async_futex);
	while (!list_empty(&fcv->waiters)) {
		tmp = fcv->waiters.next;
		f = list_get_instance(tmp, fibril_t, link);
		list_remove(&f->link);
		fibril_add_ready((fid_t) f);
		if (once)
			break;
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

/** @}
 */
