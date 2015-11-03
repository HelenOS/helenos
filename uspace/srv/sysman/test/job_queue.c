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
#include "../job_queue.h"
#include "../sysman.h"

PCUT_INIT

static bool initialized = false;

static fid_t fibril_event_loop;

static void job_finished_cb(void *object, void *arg)
{
	job_t *job = object;
	job_t **job_ptr = arg;

	/* Pass job reference to the caller */
	*job_ptr = job;
}

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

	int rc = sysman_run_job(u, STATE_STARTED, &job_finished_cb,
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

	int rc = sysman_run_job(u, STATE_STARTED, &job_finished_cb, &job);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();
	PCUT_ASSERT_EQUALS(STATE_STARTING, u->state);

	sysman_raise_event(&sysman_event_unit_exposee_created, u);
	sysman_process_queue();

	PCUT_ASSERT_NOT_NULL(job);
	PCUT_ASSERT_EQUALS(STATE_STARTED, u->state);

	job_del_ref(&job);
}

PCUT_TEST(multipath_to_started_unit) {
	/* Setup mock behavior */
	unit_type_vmts[UNIT_SERVICE]->start = &mock_unit_vmt_start_sync;

	unit_type_vmts[UNIT_MOUNT]->start = &mock_unit_vmt_start_async;
	unit_type_vmts[UNIT_MOUNT]->exposee_created =
	    &mock_unit_vmt_exposee_created;

	/* Define mock units */
	unit_t *s0 = mock_units[UNIT_SERVICE][0];
	unit_t *s1 = mock_units[UNIT_SERVICE][1];
	unit_t *m0 = mock_units[UNIT_MOUNT][0];

	/* All services require root fs */
	mock_add_edge(s0, m0);
	mock_add_edge(s1, m0);
	
	/* S1 requires another mount and S0 */
	mock_add_edge(s1, s0);

	/* Enforce initial state */
	m0->state = STATE_STARTED;

	/* Run test */
	job_t *job = NULL;
	int rc = sysman_run_job(s1, STATE_STARTED, &job_finished_cb, &job);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();

	PCUT_ASSERT_NOT_NULL(job);
	PCUT_ASSERT_EQUALS(STATE_STARTED, s0->state);
	PCUT_ASSERT_EQUALS(STATE_STARTED, s1->state);
}

PCUT_TEST(merge_jobs_with_callback) {
	/* Setup mock behavior */
	unit_type_vmts[UNIT_SERVICE]->start = &mock_unit_vmt_start_async;
	unit_type_vmts[UNIT_SERVICE]->exposee_created =
	    &mock_unit_vmt_exposee_created;

	/* Define mock units */
	unit_t *s0 = mock_units[UNIT_SERVICE][0];

	/* Create and start first job */
	job_t *j0 = NULL;
	int rc = sysman_run_job(s0, STATE_STARTED, &job_finished_cb, &j0);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();
	/* Job not finished */
	PCUT_ASSERT_NULL(j0);


	/*
	 * While s0 is starting in j0, create second job that should be merged
	 * into the existing one.
	 */
	job_t *j1 = NULL;
	rc = sysman_run_job(s0, STATE_STARTED, &job_finished_cb, &j1);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	sysman_process_queue();
	/* Job not finished */
	PCUT_ASSERT_NULL(j1);

	sysman_raise_event(&sysman_event_unit_exposee_created, s0);
	sysman_process_queue();

	PCUT_ASSERT_NOT_NULL(j0);
	PCUT_ASSERT_NOT_NULL(j1);
	
	/*
	 * Jobs were, merged so both callbacks should have been called with the
	 * same job
	 */
	PCUT_ASSERT_EQUALS(j0, j1);

	/* And there should be exactly two references (per each callback) */
	job_del_ref(&j0);
	job_del_ref(&j0);

	PCUT_ASSERT_NULL(j0);
}


PCUT_EXPORT(job_queue);
