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
#include <pcut/pcut.h>
#include <stdio.h>

#include "../job.h"

#include "mock_unit.h"

PCUT_INIT

	
static dyn_array_t exp_closure;
static dyn_array_t act_closure;

static bool same_job(job_t *expected, job_t *actual)
{
	return (expected->unit == actual->unit) &&
	    (expected->target_state == actual->target_state);
}

static bool same_jobs(dyn_array_t *expected, dyn_array_t *actual)
{
	if (expected->size != actual->size) {
		printf("%s: |expected| - |actual| = %u\n",
		    __func__, expected->size - actual->size);
		return false;
	}

	/* Verify expected \subseteq actual (we've compared sizes) */
	dyn_array_foreach(*expected, job_t *, it_exp) {
		bool found = false;
		dyn_array_foreach(*actual, job_t *, it_act) {
			if (same_job(*it_exp, *it_act)) {
				found = true;
				break;
			}
		}
		if (!found) {
			printf("%s: expected job for %s\n",
			    __func__, unit_name((*it_exp)->unit));
			return false;
		}
	}

	return true;
}

static bool job_blocked(job_t *blocked_job, job_t *blocking_job)
{
	bool found = false;
	dyn_array_foreach(blocking_job->blocked_jobs, job_t *, it) {
		if (*it == blocked_job) {
			found = true;
			break;
		}
	}
	return found;
}

static job_t *dummy_job(unit_t *unit, unit_state_t target_state)
{
	job_t *result = job_create(unit, target_state);
	return result;
}

static void dummy_add_closure(dyn_array_t *closure)
{
	dyn_array_foreach(*closure, job_t *, it) {
		(*it)->unit->job = *it;
	}
}

static void destroy_job_closure(dyn_array_t *closure)
{
	dyn_array_foreach(*closure, job_t *, it) {
		job_del_ref(&(*it));
	}
}

PCUT_TEST_SUITE(job_closure);

PCUT_TEST_BEFORE {
	mock_create_units();
	mock_set_units_state(STATE_STOPPED);

	dyn_array_initialize(&exp_closure, job_t *);
	int rc = dyn_array_reserve(&exp_closure, MAX_TYPES * MAX_UNITS);
	assert(rc == EOK);

	dyn_array_initialize(&act_closure, job_t *);
	rc = dyn_array_reserve(&act_closure, MAX_TYPES * MAX_UNITS);
	assert(rc == EOK);
}

PCUT_TEST_AFTER {
	destroy_job_closure(&act_closure);
	dyn_array_destroy(&act_closure);

	destroy_job_closure(&exp_closure);
	dyn_array_destroy(&exp_closure);

	mock_destroy_units();
}

PCUT_TEST(job_closure_linear) {
	unit_t *u0 = mock_units[UNIT_SERVICE][0];
	unit_t *u1 = mock_units[UNIT_SERVICE][1];
	unit_t *u2 = mock_units[UNIT_SERVICE][2];
	unit_t *u3 = mock_units[UNIT_SERVICE][3];

	/*
	 * u0 -> u1 -> u2 -> u3
	 */
	mock_add_dependency(u0, u1);
	mock_add_dependency(u1, u2);
	mock_add_dependency(u2, u3);

	/* Intentionally omit u0 */
	job_t *main_job = job_create(u1, STATE_STARTED);
	assert(main_job);

	int rc = job_create_closure(main_job, &act_closure);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	dyn_array_append(&exp_closure, job_t *, dummy_job(u1, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u2, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u3, STATE_STARTED));

	dummy_add_closure(&act_closure);

	PCUT_ASSERT_TRUE(same_jobs(&exp_closure, &act_closure));
	PCUT_ASSERT_TRUE(job_blocked(u1->job, u2->job));
	PCUT_ASSERT_TRUE(job_blocked(u2->job, u3->job));
}

PCUT_TEST(job_closure_fork) {
	unit_t *u0 = mock_units[UNIT_SERVICE][0];
	unit_t *u1 = mock_units[UNIT_SERVICE][1];
	unit_t *u2 = mock_units[UNIT_SERVICE][2];
	unit_t *u3 = mock_units[UNIT_SERVICE][3];

	/*
	 * u0 -> u1 ->  u2
	 *          \-> u3
	 */
	mock_add_dependency(u0, u1);
	mock_add_dependency(u1, u2);
	mock_add_dependency(u1, u3);

	job_t *main_job = job_create(u1, STATE_STARTED);
	assert(main_job);

	int rc = job_create_closure(main_job, &act_closure);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	dyn_array_append(&exp_closure, job_t *, dummy_job(u1, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u2, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u3, STATE_STARTED));

	dummy_add_closure(&act_closure);

	PCUT_ASSERT_TRUE(same_jobs(&exp_closure, &act_closure));
	PCUT_ASSERT_TRUE(job_blocked(u1->job, u2->job));
	PCUT_ASSERT_TRUE(job_blocked(u1->job, u3->job));
}

PCUT_TEST(job_closure_triangle) {
	unit_t *u0 = mock_units[UNIT_SERVICE][0];
	unit_t *u1 = mock_units[UNIT_SERVICE][1];
	unit_t *u2 = mock_units[UNIT_SERVICE][2];
	unit_t *u3 = mock_units[UNIT_SERVICE][3];

	/*
	 * u0 -> u1 ->  u2
	 *         \     v
	 *          \-> u3
	 */
	mock_add_dependency(u0, u1);
	mock_add_dependency(u1, u2);
	mock_add_dependency(u1, u3);
	mock_add_dependency(u2, u3);

	job_t *main_job = job_create(u1, STATE_STARTED);
	assert(main_job);

	int rc = job_create_closure(main_job, &act_closure);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);

	dyn_array_append(&exp_closure, job_t *, dummy_job(u1, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u2, STATE_STARTED));
	dyn_array_append(&exp_closure, job_t *, dummy_job(u3, STATE_STARTED));

	dummy_add_closure(&act_closure);

	PCUT_ASSERT_TRUE(same_jobs(&exp_closure, &act_closure));
	PCUT_ASSERT_TRUE(job_blocked(u1->job, u2->job));
	PCUT_ASSERT_TRUE(job_blocked(u1->job, u3->job));
	PCUT_ASSERT_TRUE(job_blocked(u2->job, u3->job));
}


PCUT_EXPORT(job_closure);
