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

static void job_init(job_t *job, unit_t *u, unit_state_t target_state)
{
	assert(job);
	assert(u);

	link_initialize(&job->job_queue);

	/* Start with one reference for the creator */
	atomic_set(&job->refcnt, 1);

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

static bool job_is_runnable(job_t *job)
{
	return job->state == JOB_QUEUED && job->blocking_jobs == 0;
}

static void job_check(void *object, void *data)
{
	unit_t *u = object;
	job_t *job = data;

	if (job_eval_retval(job)) {
		job_finish(job);
	} else {
		// TODO place for timeout
		// TODO add reference to job?
		sysman_object_observer(u, &job_check, job);
	}
}


static void job_unblock(job_t *blocked_job, job_t *blocking_job)
{
	if (blocking_job->retval == JOB_FAILED) {
		blocked_job->blocking_job_failed = true;
	}
	blocked_job->blocking_jobs -= 1;
}

static void job_destroy(job_t **job_ptr)
{
	job_t *job = *job_ptr;
	if (job == NULL) {
		return;
	}

	assert(!link_used(&job->job_queue));
	dyn_array_destroy(&job->blocked_jobs);
	// TODO I should decrease referece of blocked jobs

	free(job);
	*job_ptr = NULL;
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
			 * multiple jobs for one unit in the queue.
			 */
			if ((*new_job_it)->unit == queued_job->unit) {
				sysman_log(LVL_ERROR,
				    "Cannot queue multiple jobs foor unit '%s'",
				    unit_name((*new_job_it)->unit));
				return EEXIST;
			}
		}
	}

	/* Enqueue jobs */
	dyn_array_foreach(*jobs, job_ptr_t, job_it) {
		(*job_it)->state = JOB_QUEUED;
		list_append(&(*job_it)->job_queue, &job_queue);
		// TODO explain this reference
		job_add_ref(*job_it);
	}

	return EOK;
}

/** Pop next runnable job
 *
 * @return runnable job or NULL when there's none
 */
job_t *job_queue_pop_runnable(void)
{
	job_t *result = NULL;
	link_t *first_link = list_first(&job_queue);
	bool first_iteration = true;

	list_foreach_safe(job_queue, cur_link, next_link) {
		result = list_get_instance(cur_link, job_t, job_queue);
		if (job_is_runnable(result)) {
			break;
		} else if (!first_iteration && cur_link == first_link) {
			result = NULL;
			break;
		} else {
			/*
			 * We make no assuptions about ordering of jobs in the
			 * queue, so just move the job to the end of the queue.
			 * If there are exist topologic ordering, eventually
			 * jobs will be reordered. Furthermore when if there
			 * exists any runnable job, it's always found.
			 */
			list_remove(cur_link);
			list_append(cur_link, &job_queue);
		}
		first_iteration = false;
	}

	if (result) {
		// TODO update refcount
		list_remove(&result->job_queue);
		result->state = JOB_DEQUEUED;
	}

	return result;
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
	 */
	fifo_push(jobs_fifo, main_job);
	job_t *job;
	while ((job = fifo_pop(jobs_fifo)) != NULL) {
		/*
		 * Do not increase reference count of job, as we're passing it
		 * to the closure.
		 */
		dyn_array_append(job_closure, job_ptr_t, job);

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
	 * deleting them here.
	 */
	return rc;
}

job_t *job_create(unit_t *u, unit_state_t target_state)
{
	job_t *job = malloc(sizeof(job_t));
	if (job != NULL) {
		job_init(job, u, target_state);
	}

	return job;
}

void job_add_ref(job_t *job)
{
	atomic_inc(&job->refcnt);
}

void job_del_ref(job_t **job_ptr)
{
	job_t *job = *job_ptr;
	if (job == NULL) {
		return;
	}

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
	sysman_log(LVL_DEBUG, "%s, %s -> %i",
	    __func__, unit_name(u), job->target_state);

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

	sysman_log(LVL_DEBUG2, "%s(%s) -> %i",
	    __func__, unit_name(job->unit), job->retval);

	job->state = JOB_FINISHED;

	/* Job finished */
	dyn_array_foreach(job->blocked_jobs, job_ptr_t, job_it) {
		job_unblock(*job_it, job);
	}
	// TODO remove reference from blocked jobs

	// TODO should reference be added (for the created event)
	sysman_raise_event(&sysman_event_job_changed, job);
}

