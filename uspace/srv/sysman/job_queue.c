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

#include "job_queue.h"
#include "log.h"
#include "sysman.h"

static list_t job_queue;

/*
 * Static functions
 */

static bool job_is_runnable(job_t *job)
{
	assert(job->state == JOB_PENDING);
	return job->blocking_jobs == 0;
}

/** Pop next runnable job
 *
 * @return runnable job or NULL when there's none
 */
static job_t *job_queue_pop_runnable(void)
{
	job_t *result = NULL;

	/* Select first runnable job */
	list_foreach(job_queue, job_queue, job_t, candidate) {
		if (job_is_runnable(candidate)) {
			result = candidate;
			break;
		}
	}
	if (result) {
		/* Remove job from queue and pass reference to caller */
		list_remove(&result->job_queue);
	}

	return result;
}

/** Add multiple references to job
 *
 * Non-atomicity doesn't mind as long as individual increments are atomic.
 *
 * @note Function is not exported as other modules shouldn't need it.
 */
static inline void job_add_refs(job_t *job, size_t refs)
{
	for (size_t i = 0; i < refs; ++i) {
		job_add_ref(job);
	}
}

/** Delete multiple references to job
 *
 * Behavior of concurrent runs with job_add_refs aren't specified.
 */
static inline void job_del_refs(job_t **job_ptr, size_t refs)
{
	for (size_t i = 0; i < refs; ++i) {
		job_del_ref(job_ptr);
	}
}

/** Merge two jobs together
 *
 * @param[in/out]  trunk  job that
 * @param[in]      other  job that will be cleared out
 *
 * @return EOK on success
 * @return error code on fail
 */
static void job_pre_merge(job_t *trunk, job_t *other)
{
	assert(trunk->unit == other->unit);
	assert(trunk->target_state == other->target_state);
	assert(list_count(&trunk->blocked_jobs) == trunk->blocked_jobs_count);
	assert(other->merged_into == NULL);

	list_concat(&trunk->blocked_jobs, &other->blocked_jobs);

	while (!list_empty(&other->blocked_jobs)) {
		job_link_t *job_link = list_pop(&other->blocked_jobs, job_link_t, link);
		free(job_link);
	}

	// TODO allocate observed object

	other->merged_into = trunk;
}

static void job_finish_merge(job_t *trunk, job_t *other)
{
	assert(list_count(&trunk->blocked_jobs) >= trunk->blocked_jobs_count);
	//TODO aggregate merged blocked_jobs
	trunk->blocked_jobs_count = list_count(&other->blocked_jobs);

	/*
	 * Note, the sysman_move_observers cannot fail here sice all necessary
	 * allocation is done in job_pre_merge.
	 */
	size_t observers_refs = sysman_observers_count(other);
	errno_t rc = sysman_move_observers(other, trunk);
	assert(rc == EOK);

	/* When we move observers, don't forget to pass their references too. */
	job_add_refs(trunk, observers_refs);
	job_del_refs(&other, observers_refs);
}

static void job_undo_merge(job_t *trunk)
{
	unsigned long count = list_count(&trunk->blocked_jobs);
	assert(count >= trunk->blocked_jobs_count);

	while (count-- > 0) {
		link_t *last_link = list_last(&trunk->blocked_jobs);
		job_link_t *job_link = list_get_instance(last_link, job_link_t, link);
		list_remove(last_link);
		free(job_link);
	}
}

/*
 * Non-static functions
 */

void job_queue_init()
{
	list_initialize(&job_queue);
}

/** Consistenly add jobs to the queue
 *
 * @param[in/out]  closure    jobs closure, on success it's emptied, otherwise
 *                            you should take care of remaining jobs
 *
 * @return EOK on success
 * @return EBUSY when any job in closure is conflicting
 */
errno_t job_queue_add_closure(job_closure_t *closure)
{
	bool has_error = false;
	errno_t rc = EOK;

	/* Check consistency with existing jobs. */
	list_foreach(*closure, link, job_link_t, job_it) {
		job_t *job = job_it->job;
		job_t *other_job = job->unit->job;

		if (other_job == NULL) {
			continue;
		}

		if (other_job->target_state != job->target_state) {
			switch (other_job->state) {
			case JOB_RUNNING:
				sysman_log(LVL_ERROR,
				    "Unit '%s' has already different job running.",
				    unit_name(job->unit));
				has_error = true;
				continue;
			case JOB_PENDING:
				/*
				 * Currently we have strict strategy not
				 * permitting multiple jobs for one unit in the
				 * queue at a time.
				 */
				sysman_log(LVL_ERROR,
				    "Cannot queue multiple jobs for unit '%s'.",
				    unit_name(job->unit));
				has_error = true;
				continue;
			default:
				assert(false);
			}
		} else {
			// TODO think about other options to merging
			//      (replacing, cancelling)
			job_pre_merge(other_job, job);
		}
	}

	/* Aggregate merged jobs, or rollback any changes in existing jobs */
	bool finish_merge = (rc == EOK) && !has_error;
	list_foreach(*closure, link, job_link_t, job_it) {
		job_t *job = job_it->job;
		if (job->merged_into == NULL) {
			continue;
		}
		if (finish_merge) {
			job_finish_merge(job->merged_into, job);
		} else {
			job_undo_merge(job->merged_into);
		}
	}
	if (has_error) {
		return EBUSY;
	} else if (rc != EOK) {
		return rc;
	}

	/*
	 * Unmerged jobs are enqueued, merged are disposed
	 *
	 * TODO Ensure that jobs that block merged jobs contain the corrent job
	 *      in their blocked_jobs array.
	 */
	list_foreach(*closure, link, job_link_t, job_it) {
		job_t *job = job_it->job;
		if (job->merged_into != NULL) {
			job_del_ref(&job);
			continue;
		}

		unit_t *u = job->unit;
		assert(u->job == NULL);
		/* Pass reference from the closure to the unit */
		u->job = job;

		/* Enqueue job (new reference) */
		job->state = JOB_PENDING;
		job_add_ref(job);
		list_append(&job->job_queue, &job_queue);
	}

	/* We've stolen references from the closure, so erase it */
	while (!list_empty(closure)) {
		job_link_t *job_link = list_pop(closure, job_link_t, link);
		free(job_link);
	}

	return EOK;
}

/** Process all jobs that aren't transitively blocked
 *
 * Job can be blocked either by another job or by an incoming event, that will
 * be queued after this job_queue_process call.
 *
 */
void job_queue_process(void)
{
	job_t *job;
	while ((job = job_queue_pop_runnable())) {
		job_run(job);
		job_del_ref(&job);
	}
}
