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

#include "repo.h"
#include "edge.h"
#include "job.h"
#include "log.h"
#include "sysman.h"

static list_t job_queue;

/*
 * Static functions
 */

static int job_add_blocked_job(job_t *blocking_job, job_t *blocked_job)
{
	assert(blocking_job->blocked_jobs.size ==
    	    blocking_job->blocked_jobs_count);

	int rc = dyn_array_append(&blocking_job->blocked_jobs, job_t *,
	    blocked_job);
	if (rc != EOK) {
		return ENOMEM;
	}
	job_add_ref(blocked_job);

	blocking_job->blocked_jobs_count += 1;
	blocked_job->blocking_jobs += 1;

	return EOK;
}

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

	atomic_set(&job->refcnt, 0);

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
static int job_pre_merge(job_t *trunk, job_t *other)
{
	assert(trunk->unit == other->unit);
	assert(trunk->target_state == other->target_state);
	assert(trunk->blocked_jobs.size == trunk->blocked_jobs_count);
	assert(other->merged_into == NULL);

	int rc = dyn_array_concat(&trunk->blocked_jobs, &other->blocked_jobs);
	if (rc != EOK) {
		return rc;
	}
	dyn_array_clear(&other->blocked_jobs);

	// TODO allocate observed object

	other->merged_into = trunk;

	return EOK;
}

static void job_finish_merge(job_t *trunk, job_t *other)
{
	assert(trunk->blocked_jobs.size >= trunk->blocked_jobs_count);
	//TODO aggregate merged blocked_jobs
	trunk->blocked_jobs_count = other->blocked_jobs.size;

	/*
	 * Note, the sysman_move_observers cannot fail here sice all necessary
	 * allocation is done in job_pre_merge.
	 */
	size_t observers_refs = sysman_observers_count(other);
	int rc = sysman_move_observers(other, trunk);
	assert(rc == EOK);

	/* When we move observers, don't forget to pass their references too. */
	job_add_refs(trunk, observers_refs);
	job_del_refs(&other, observers_refs);
}

static void job_undo_merge(job_t *trunk)
{
	assert(trunk->blocked_jobs.size >= trunk->blocked_jobs_count);
	dyn_array_clear_range(&trunk->blocked_jobs,
	    trunk->blocked_jobs_count, trunk->blocked_jobs.size);
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
int job_queue_add_closure(dyn_array_t *closure)
{
	bool has_error = false;
	int rc = EOK;

	/* Check consistency with existing jobs. */
	dyn_array_foreach(*closure, job_t *, job_it) {
		job_t *job = *job_it;
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
			rc = job_pre_merge(other_job, job);
			if (rc != EOK) {
				break;
			}
		}
	}

	/* Aggregate merged jobs, or rollback any changes in existing jobs */
	bool finish_merge = (rc == EOK) && !has_error;
	dyn_array_foreach(*closure, job_t *, job_it) {
		if ((*job_it)->merged_into == NULL) {
			continue;
		}
		if (finish_merge) {
			job_finish_merge((*job_it)->merged_into, *job_it);
		} else {
			job_undo_merge((*job_it)->merged_into);
		}
	}
	if (has_error) {
		return EBUSY;
	} else if (rc != EOK) {
		return rc;
	}

	/* Unmerged jobs are enqueued, merged are disposed */
	dyn_array_foreach(*closure, job_t *, job_it) {
		job_t *job = (*job_it);
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
	dyn_array_clear(closure);

	return EOK;
}

/** Process all jobs that aren't transitively blocked
 *
 * Job can be blocked either by another job or by an incoming event, that will
 * be queued after this job_queue_process call.
 *
 * TODO Write down rules from where this function can be called, to avoid stack
 *      overflow.
 */
void job_queue_process(void)
{
	job_t *job;
	while ((job = job_queue_pop_runnable())) {
		job_run(job);
		job_del_ref(&job);
	}
}

int job_create_closure(job_t *main_job, dyn_array_t *job_closure)
{
	sysman_log(LVL_DEBUG2, "%s(%s)", __func__, unit_name(main_job->unit));
	int rc;
	list_t units_fifo;
	list_initialize(&units_fifo);

	/* Check invariant */
	list_foreach(units, units, unit_t, u) {
		assert(u->bfs_job == NULL);
	}
		
	unit_t *unit = main_job->unit;
	job_add_ref(main_job);
	unit->bfs_job = main_job;
	list_append(&unit->bfs_link, &units_fifo);
	
	while (!list_empty(&units_fifo)) {
		unit = list_get_instance(list_first(&units_fifo), unit_t,
		    bfs_link);
		list_remove(&unit->bfs_link);
		job_t *job = unit->bfs_job;
		assert(job != NULL);

		job_add_ref(job);
		dyn_array_append(job_closure, job_t *, job);

		/*
		 * Traverse dependencies edges
		 * According to dependency type and edge direction create
		 * appropriate jobs (currently "After" only).
		 */
		list_foreach(unit->edges_out, edges_out, unit_edge_t, e) {
			unit_t *u = e->output;
			job_t *blocking_job;

			if (u->bfs_job == NULL) {
				blocking_job = job_create(u, job->target_state);
				if (blocking_job == NULL) {
					rc = ENOMEM;
					goto finish;
				}
				/* Pass reference to unit */
				u->bfs_job = blocking_job;
				list_append(&u->bfs_link, &units_fifo);
			} else {
				blocking_job = u->bfs_job;
			}

			job_add_blocked_job(blocking_job, job);
		}
	}
	sysman_log(LVL_DEBUG2, "%s(%s):", __func__, unit_name(main_job->unit));
	dyn_array_foreach(*job_closure, job_t *, job_it) {
		sysman_log(LVL_DEBUG2, "%s\t%s, refs: %u", __func__,
		    unit_name((*job_it)->unit), atomic_get(&(*job_it)->refcnt));
	}
	rc = EOK;

finish:
	/* Unreference any jobs in interrupted BFS queue */
	list_foreach_safe(units_fifo, cur_link, next_link) {
		unit_t *u = list_get_instance(cur_link, unit_t, bfs_link);
		job_del_ref(&u->bfs_job);
		list_remove(cur_link);
	}
	
	/* Clean after ourselves (BFS tag jobs) */
	dyn_array_foreach(*job_closure, job_t *, job_it) {
		assert(*job_it == (*job_it)->unit->bfs_job);
		job_del_ref(&(*job_it)->unit->bfs_job);
		(*job_it)->unit->bfs_job = NULL;
	}

	return rc;
}

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
	atomic_inc(&job->refcnt);
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
	assert(atomic_get(&job->refcnt) > 0);
	if (atomic_predec(&job->refcnt) == 0) {
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

	int rc;
	switch (job->target_state) {
	case STATE_STARTED:
		// TODO put here same evaluation as in job_check
		//      goal is to have job_run "idempotent"
		if (u->state == job->target_state) {
			rc = EOK;
		} else {
			rc = unit_start(u);
		}
		break;
	default:
		// TODO implement other states
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

	sysman_log(LVL_DEBUG2, "%s(%p) %s ret %i, ref %i",
	    __func__, job, unit_name(job->unit), job->retval,
	    atomic_get(&job->refcnt));

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

