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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>

#include "repo.h"
#include "edge.h"
#include "job.h"
#include "log.h"
#include "sysman.h"

/*
 * Static functions
 */

/** Remove blocking_job from blocked job structure
 *
 * @note Caller must remove blocked_job from collection of blocked_jobs
 */
static void job_unblock(job_t *blocked_job, job_t *blocking_job)
{
	if (blocking_job->retval == JOB_FAILED) {
		blocked_job->blocking_job_failed = true;
	}
	blocked_job->blocking_jobs -= 1;

	job_del_ref(&blocked_job);
}

static void job_init(job_t *job, unit_t *u, unit_state_t target_state)
{
	assert(job);
	assert(u);
	memset(job, 0, sizeof(*job));

	link_initialize(&job->job_queue);

	atomic_store(&job->refcnt, 0);

	job->target_state = target_state;
	job->unit = u;

	dyn_array_initialize(&job->blocked_jobs, job_t *);
	job->blocking_jobs = 0;
	job->blocking_job_failed = false;

	job->state = JOB_EMBRYO;
	job->retval = JOB_UNDEFINED_;
}

static bool job_eval_retval(job_t *job)
{
	unit_t *u = job->unit;

	if (u->state == job->target_state) {
		job->retval = JOB_OK;
		return true;
	} else if (u->state == STATE_FAILED) {
		job->retval = JOB_FAILED;
		return true;
	} else {
		return false;
	}
}

static void job_check(void *object, void *data)
{
	unit_t *u = object;
	job_t *job = data;

	/*
	 * We have one reference from caller for our disposal,
	 * if needed, pass it to observer.
	 */
	if (job_eval_retval(job)) {
		job_finish(job);
		job_del_ref(&job);
	} else {
		// TODO place for timeout
		sysman_object_observer(u, &job_check, job);
	}
}

static void job_destroy(job_t **job_ptr)
{
	job_t *job = *job_ptr;
	if (job == NULL) {
		return;
	}

	assert(!link_used(&job->job_queue));

	dyn_array_foreach(job->blocked_jobs, job_t *, job_it) {
		job_del_ref(&(*job_it));
	}
	dyn_array_destroy(&job->blocked_jobs);

	free(job);
	*job_ptr = NULL;
}

/*
 * Non-static functions
 */

/** Create job assigned to the unit
 *
 * @param[in]  unit
 * @param[in]  target_state
 *
 * @return NULL or newly created job (there is a single refernce for the creator)
 */
job_t *job_create(unit_t *u, unit_state_t target_state)
{
	job_t *job = malloc(sizeof(job_t));
	if (job != NULL) {
		job_init(job, u, target_state);

		/* Add one reference for the creator */
		job_add_ref(job);
	}

	return job;
}

/** Add one reference to job
 *
 * Usage:
 *   - adding observer which references the job,
 *   - raising and event that references the job,
 *   - anytime any other new reference is made.
 */
void job_add_ref(job_t *job)
{
	atomic_fetch_add(&job->refcnt, 1);
}

/** Remove one reference from job, last remover destroys the job
 *
 * Usage:
 *   - inside observer callback that references the job,
 *   - inside event handler that references the job,
 *   - anytime you dispose a reference to the job.
 */
void job_del_ref(job_t **job_ptr)
{
	job_t *job = *job_ptr;

	assert(job != NULL);
	assert(atomic_load(&job->refcnt) > 0);
	atomic_fetch_sub(&job->refcnt, 1);
	if (atomic_load(&job->refcnt) == 0) {
		job_destroy(job_ptr);
	}
}

void job_run(job_t *job)
{
	assert(job->state == JOB_PENDING);

	job->state = JOB_RUNNING;
	unit_t *u = job->unit;
	sysman_log(LVL_DEBUG, "%s(%p), %s -> %i",
	    __func__, job, unit_name(u), job->target_state);

	/* Propagate failure */
	if (job->blocking_job_failed) {
		goto fail;
	}

	errno_t rc;
	// TODO put here similar evaluation as in job_check
	//      goal is to have job_run "idempotent"
	switch (job->target_state) {
	case STATE_STARTED:
		if (u->state == job->target_state) {
			rc = EOK;
		} else {
			rc = unit_start(u);
		}
		break;
	case STATE_STOPPED:
		if (u->state == job->target_state) {
			rc = EOK;
		} else {
			rc = unit_stop(u);
		}
		break;
	default:
		assert(false);
	}
	if (rc != EOK) {
		//TODO here is 'rc' value "lost" (not propagated further)
		sysman_log(LVL_DEBUG, "%s(%p), %s -> %i, error: %i",
		    __func__, job, unit_name(u), job->target_state, rc);
		goto fail;
	}

	/*
	 * job_check deletes reference, we want job to remain to caller, thus
	 * add one dummy ref
	 */
	job_add_ref(job);
	job_check(job->unit, job);
	return;

fail:
	job->retval = JOB_FAILED;
	job_finish(job);
}

/** Unblocks blocked jobs and notify observers
 *
 * @param[in]  job  job with defined return value
 */
void job_finish(job_t *job)
{
	assert(job->state != JOB_FINISHED);
	assert(job->retval != JOB_UNDEFINED_);
	assert(!job->unit->job || job->unit->job == job);

	sysman_log(LVL_DEBUG2, "%s(%p) %s ret %i, ref %u",
	    __func__, job, unit_name(job->unit), job->retval,
	    atomic_load(&job->refcnt));

	job->state = JOB_FINISHED;

	/* First remove references, then clear the array */
	assert(job->blocked_jobs.size == job->blocked_jobs_count);
	dyn_array_foreach(job->blocked_jobs, job_t *, job_it) {
		job_unblock(*job_it, job);
	}
	dyn_array_clear(&job->blocked_jobs);

	/* Add reference for event handler */
	if (job->unit->job == NULL) {
		job_add_ref(job);
	} else {
		/* Pass reference from unit */
		job->unit->job = NULL;
	}
	sysman_raise_event(&sysman_event_job_finished, job);
}
