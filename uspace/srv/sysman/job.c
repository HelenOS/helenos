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

#include <adt/fifo.h>
#include <assert.h>
#include <errno.h>

#include "dep.h"
#include "job.h"
#include "log.h"
#include "sysman.h"

static list_t job_queue;

/*
 * Static functions
 */

static int job_add_blocked_job(job_t *job, job_t *blocked_job)
{
	int rc = dyn_array_append(&job->blocked_jobs, job_ptr_t, blocked_job);
	if (rc != EOK) {
		return ENOMEM;
	}
	job_add_ref(blocked_job);

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

	dyn_array_initialize(&job->blocked_jobs, job_ptr_t, 0);
	job->blocking_jobs = 0;
	job->blocking_job_failed = false;

	job->state = JOB_UNQUEUED;
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
	 * We have one reference from caller for our disposal,	 *
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

	dyn_array_foreach(job->blocked_jobs, job_ptr_t, job_it) {
		job_del_ref(&(*job_it));
	}
	dyn_array_destroy(&job->blocked_jobs);

	free(job);
	*job_ptr = NULL;
}

static bool job_is_runnable(job_t *job)
{
	return job->state == JOB_QUEUED && job->blocking_jobs == 0;
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
		result->state = JOB_DEQUEUED;
	}

	return result;
}

/*
 * Non-static functions
 */

void job_queue_init()
{
	list_initialize(&job_queue);
}

int job_queue_add_jobs(dyn_array_t *jobs)
{
	/* Check consistency with queue. */
	dyn_array_foreach(*jobs, job_ptr_t, new_job_it) {
		list_foreach(job_queue, job_queue, job_t, queued_job) {
			/*
			 * Currently we have strict strategy not permitting
			 * multiple jobs for one unit in the queue at a time.
			 */
			if ((*new_job_it)->unit == queued_job->unit) {
				sysman_log(LVL_ERROR,
				    "Cannot queue multiple jobs for unit '%s'",
				    unit_name((*new_job_it)->unit));
				return EEXIST;
			}
		}
	}

	/* Enqueue jobs */
	dyn_array_foreach(*jobs, job_ptr_t, job_it) {
		(*job_it)->state = JOB_QUEUED;
		list_append(&(*job_it)->job_queue, &job_queue);
		/* We pass reference from the closure to the queue */
	}

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
	// TODO replace hard-coded FIFO size with resizable FIFO
	FIFO_INITIALIZE_DYNAMIC(jobs_fifo, job_ptr_t, 10);
	void *fifo_data = fifo_create(jobs_fifo);
	int rc;
	if (fifo_data == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	/*
	 * Traverse dependency graph in BFS fashion and create jobs for every
	 * necessary unit.
	 * Closure keeps reference to each job. We've to add reference to the
	 * main job, other newly created jobs are pased to the closure.
	 */
	fifo_push(jobs_fifo, main_job);
	job_add_ref(main_job);
	while (jobs_fifo.head != jobs_fifo.tail) {
		job_t *job = fifo_pop(jobs_fifo);
		
		// TODO more sophisticated check? (unit that is in transitional
		//      state cannot have currently multiple jobs queued)
		if (job->target_state == job->unit->state) {
			/*
			 * Job would do nothing, finish it on spot.
			 * No need to continue BFS search from it.
			 */
			job->retval = JOB_OK;
			job_finish(job);
			job_del_ref(&job);
			continue;
		} else {
			/* No refcount increase, pass it to the closure */
			dyn_array_append(job_closure, job_ptr_t, job);
		}

		/* Traverse dependencies edges */
		unit_t *u = job->unit;
		list_foreach(u->dependencies, dependencies, unit_dependency_t, dep) {
			// TODO prepare for reverse edge direction and
			//      non-identity state mapping
			job_t *new_job =
			    job_create(dep->dependency, job->target_state);
			if (new_job == NULL) {
				rc = ENOMEM;
				goto finish;
			}
			job_add_blocked_job(new_job, job);
			fifo_push(jobs_fifo, new_job);
		}
	}
	rc = EOK;

finish:
	free(fifo_data);
	/*
	 * Newly created jobs are already passed to the closure, thus not
	 * deleting reference to them here.
	 */
	return rc;
}

job_t *job_create(unit_t *u, unit_state_t target_state)
{
	job_t *job = malloc(sizeof(job_t));
	if (job != NULL) {
		job_init(job, u, target_state);

		/* Start with one reference for the creator */
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
	assert(job->state != JOB_RUNNING);
	assert(job->state != JOB_FINISHED);

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
		rc = unit_start(u);
		break;
	default:
		// TODO implement other states
		assert(false);
	}
	if (rc != EOK) {
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

	sysman_log(LVL_DEBUG2, "%s(%p) %s -> %i",
	    __func__, job, unit_name(job->unit), job->retval);

	job->state = JOB_FINISHED;

	/* First remove references, then clear the array */
	dyn_array_foreach(job->blocked_jobs, job_ptr_t, job_it) {
		job_unblock(*job_it, job);
	}
	dyn_array_clear(&job->blocked_jobs);

	/* Add reference for the event */
	job_add_ref(job);
	sysman_raise_event(&sysman_event_job_finished, job);
}

