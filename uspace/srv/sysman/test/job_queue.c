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

#include <assert.h>
#include <fibril.h>
#include <pcut/pcut.h>
#include <stdbool.h>

#include "mock_unit.h"
#include "../job.h"
#include "../sysman.h"

PCUT_INIT

static bool initialized = false;

static fid_t fibril_event_loop;

#if 0
static bool async_finished;
static fibril_condvar_t async_finished_cv;
static fibril_mutex_t async_finished_mtx;
#endif

static void async_finished_callback(void *object, void *arg)
{
	job_t *job = object;
	job_t **job_ptr = arg;

	/* Pass job reference to the caller */
	*job_ptr = job;
}

#if 0
static void reset_wait(void)
{
	fibril_mutex_lock(&async_finished_mtx);
	async_finished = false;
	fibril_mutex_unlock(&async_finished_mtx);
}

static void async_wait()
{
	fibril_mutex_lock(&async_finished_mtx);
	while (!async_finished) {
		fibril_condvar_wait(&async_finished_cv, &async_finished_mtx);
	}
	fibril_mutex_unlock(&async_finished_mtx);
}
#endif

PCUT_TEST_SUITE(job_queue);

PCUT_TEST_BEFORE {
	mock_create_units();
	mock_set_units_state(STATE_STOPPED);

	if (!initialized) {
		sysman_events_init();
		job_queue_init();

#if 0
		fibril_condvar_initialize(&async_finished_cv);
		fibril_mutex_initialize(&async_finished_mtx);
#endif

		fibril_event_loop = fibril_create(sysman_events_loop, NULL);
		fibril_add_ready(fibril_event_loop);

		initialized = true;
	}
}

PCUT_TEST_AFTER {
	mock_destroy_units();
}

PCUT_TEST(single_start_sync) {
	unit_type_vmts[UNIT_TARGET]->start = &mock_unit_vmt_start_sync;

	unit_t *u = mock_units[UNIT_TARGET][0];
	job_t *job = NULL;

	int rc = sysman_run_job(u, STATE_STARTED, &async_finished_callback,
	    &job);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();
	PCUT_ASSERT_NOT_NULL(job);
	PCUT_ASSERT_EQUALS(STATE_STARTED, u->state);

	job_del_ref(&job);
}

PCUT_TEST(single_start_async) {
	unit_type_vmts[UNIT_TARGET]->start = &mock_unit_vmt_start_async;
	unit_type_vmts[UNIT_TARGET]->exposee_created =
	    &mock_unit_vmt_exposee_created;

	unit_t *u = mock_units[UNIT_TARGET][0];
	job_t *job = NULL;

	int rc = sysman_run_job(u, STATE_STARTED, &async_finished_callback,
	    &job);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();
	PCUT_ASSERT_EQUALS(STATE_STARTING, u->state);

	sysman_raise_event(&sysman_event_unit_exposee_created, u);
	sysman_process_queue();

	PCUT_ASSERT_NOT_NULL(job);
	PCUT_ASSERT_EQUALS(STATE_STARTED, u->state);

	job_del_ref(&job);
}


PCUT_EXPORT(job_queue);
