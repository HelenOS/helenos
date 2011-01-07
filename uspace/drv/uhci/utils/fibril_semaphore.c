/*
 * Copyright (c) 2010 Jan Vesely
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
#include <async.h>
#include <async_priv.h>
#include <futex.h>

#include "fibril_semaphore.h"
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
    ipc_poke();
}
/*----------------------------------------------------------------------------*/
void fibril_semaphore_initialize(fibril_semaphore_t *fs, int value)
{
	assert( fs );
  fs->oi.owned_by = NULL;
  fs->counter = 1;
	fs->value = value;
  list_initialize(&fs->waiters);
}
/*----------------------------------------------------------------------------*/
void fibril_semaphore_down(fibril_semaphore_t *fs)
{
	assert( fs );
  fibril_t *f = (fibril_t *) fibril_get_id();

  futex_down(&async_futex);
  if (--fs->value < 0) {
    awaiter_t wdata;
    wdata.fid = fibril_get_id();
    wdata.active = false;
    wdata.wu_event.inlist = true;
    link_initialize(&wdata.wu_event.link);
    list_append(&wdata.wu_event.link, &fs->waiters);
//    check_for_deadlock(&fm->oi);
    f->waits_for = &fs->oi;
		++fs->counter;
    fibril_switch(FIBRIL_TO_MANAGER);
  } else {
    fs->oi.owned_by = f;
    futex_up(&async_futex);
  }
}
/*----------------------------------------------------------------------------*/
bool fibril_semaphore_trydown(fibril_semaphore_t *fs)
{
  bool locked = false;

  futex_down(&async_futex);
  if (fs->value > 0) {
    --fs->value;
    fs->oi.owned_by = (fibril_t *) fibril_get_id();
    locked = true;
  }
  futex_up(&async_futex);

  return locked;
}
/*----------------------------------------------------------------------------*/
void fibril_semaphore_up(fibril_semaphore_t *fs)
{
	assert(fs);
	futex_down(&async_futex);
  if (++fs->value <= 0) {
    link_t *tmp;
    awaiter_t *wdp;
    fibril_t *f;

    assert(!list_empty(&fs->waiters));

    tmp = fs->waiters.next;
    wdp = list_get_instance(tmp, awaiter_t, wu_event.link);
    wdp->active = true;
    wdp->wu_event.inlist = false;

    f = (fibril_t *) wdp->fid;
    fs->oi.owned_by = f;
    f->waits_for = NULL;

    list_remove(&wdp->wu_event.link);
    fibril_add_ready(wdp->fid);
    optimize_execution_power();
  } else {
    fs->oi.owned_by = NULL;
  }
	futex_up(&async_futex);
}
