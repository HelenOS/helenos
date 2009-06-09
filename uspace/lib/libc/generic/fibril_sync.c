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

void fibril_mutex_unlock(fibril_mutex_t *fm)
{
	futex_down(&async_futex);
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
	futex_up(&async_futex);
}

void fibril_rwlock_initialize(fibril_rwlock_t *frw)
{
	fibril_mutex_initialize(&frw->fm);
}

void fibril_rwlock_read_lock(fibril_rwlock_t *frw)
{
	fibril_mutex_lock(&frw->fm);
}

void fibril_rwlock_write_lock(fibril_rwlock_t *frw)
{
	fibril_mutex_lock(&frw->fm);
}

void fibril_rwlock_read_unlock(fibril_rwlock_t *frw)
{
	fibril_mutex_unlock(&frw->fm);
}

void fibril_rwlock_write_unlock(fibril_rwlock_t *frw)
{
	fibril_mutex_unlock(&frw->fm);
}

/** @}
 */
