/*
 * Copyright (c) 2015 Michal Koutny
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

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "../tester.h"
#include "common.h"

static task_id_t task_id;
static task_exit_t last_texit;
static bool last_has_retval;
static int last_retval;
static bool handler_hit;

/* Locks are listed in their locking order */
static FIBRIL_RWLOCK_INITIALIZE(tid_lck);
static FIBRIL_MUTEX_INITIALIZE(sync_mtx);

static FIBRIL_CONDVAR_INITIALIZE(sync_cv);

static void task_event_handler(task_id_t tid, int flags, task_exit_t texit,
    int retval)
{
	fibril_rwlock_read_lock(&tid_lck);
	fibril_mutex_lock(&sync_mtx);

	if (task_id != tid) {
		goto finish;
	}

	handler_hit = true;
finish:
	fibril_condvar_signal(&sync_cv);
	fibril_mutex_unlock(&sync_mtx);
	fibril_rwlock_read_unlock(&tid_lck);
}

static void reset_wait(bool purge)
{
	fibril_mutex_lock(&sync_mtx);
	handler_hit = false;
	if (purge) {
		last_texit = TASK_EXIT_RUNNING;
		last_has_retval = false;
		last_retval = 255;
	}
	fibril_mutex_unlock(&sync_mtx);
}

static void wait_for_handler(void)
{
	fibril_mutex_lock(&sync_mtx);
	while (!handler_hit) {
		fibril_condvar_wait(&sync_cv, &sync_mtx);
	}
	fibril_mutex_unlock(&sync_mtx);
}

static inline int safe_dummy_task_spawn(task_id_t *task_id, task_wait_t *wait,
    const char *behavior)
{
	fibril_rwlock_write_lock(&tid_lck);
	int rc = dummy_task_spawn(task_id, wait, behavior);
	fibril_rwlock_write_unlock(&tid_lck);
	return rc;
}

const char *test_proc_task_anywait(void)
{
	const char *err = NULL;

	int rc;

	task_set_event_handler(task_event_handler);

	TPRINTF("1 exit only\n");

	reset_wait(true);
	rc = safe_dummy_task_spawn(&task_id, NULL, STR_FAIL);
	TASSERT(rc == EOK);
	wait_for_handler();
	TASSERT(last_has_retval == false);
	TASSERT(last_texit == TASK_EXIT_UNEXPECTED);
	/* --- */

	TPRINTF("2 daemon + kill\n");

	reset_wait(true);
	rc = safe_dummy_task_spawn(&task_id, NULL, STR_DAEMON);
	TASSERT(rc == EOK);
	wait_for_handler();
	TASSERT(last_has_retval == true);
	TASSERT(last_retval == EOK);
	TASSERT(last_texit == TASK_EXIT_RUNNING);

	reset_wait(false);
	task_kill(task_id);
	wait_for_handler();
	TASSERT(last_texit == TASK_EXIT_UNEXPECTED);
	/* --- */

	TPRINTF("3 successful job\n");

	reset_wait(true);
	rc = safe_dummy_task_spawn(&task_id, NULL, STR_JOB_OK);
	TASSERT(rc == EOK);
	wait_for_handler(); /* job is notified in a single handler call */
	TASSERT(last_has_retval == true);
	TASSERT(last_retval == EOK);
	TASSERT(last_texit == TASK_EXIT_NORMAL);
	/* --- */

	TPRINTF("3 successful job with discrimination\n");

	reset_wait(true);
	rc = safe_dummy_task_spawn(&task_id, NULL, STR_JOB_OK);
	TASSERT(rc == EOK);
	/* spoil it with another task events */
	rc = dummy_task_spawn(NULL, NULL, STR_JOB_OK);
	TASSERT(rc == EOK);
	wait_for_handler();
	TASSERT(last_has_retval == true);
	TASSERT(last_retval == EOK);
	TASSERT(last_texit == TASK_EXIT_NORMAL);
	/* --- */


	TPRINTF("All task waiting tests finished");

	return err;
}
