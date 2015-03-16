#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <stdlib.h>

#include "job.h"
#include "unit.h"

static list_t job_queue;
static fibril_mutex_t job_queue_mtx;
static fibril_condvar_t job_queue_cv;

static void job_destroy(job_t **);


static int job_run_start(job_t *job)
{
	unit_t *unit = job->unit;

	int rc = unit_start(unit);
	if (rc != EOK) {
		return rc;
	}

	fibril_mutex_lock(&unit->state_mtx);
	while (unit->state != STATE_STARTED) {
		fibril_condvar_wait(&unit->state_cv, &unit->state_mtx);
	}
	fibril_mutex_unlock(&unit->state_mtx);

	// TODO react to failed state
	return EOK;
}

static int job_runner(void *arg)
{
	job_t *job = (job_t *)arg;

	int retval = EOK;

	/* Wait for previous jobs */
	list_foreach(job->blocking_jobs, link, job_link_t, jl) {
		retval = job_wait(jl->job);
		if (retval != EOK) {
			break;
		}
	}

	if (retval != EOK) {
		goto finish;
	}

	/* Run the job itself */
	fibril_mutex_lock(&job->state_mtx);
	job->state = JOB_RUNNING;
	fibril_condvar_broadcast(&job->state_cv);
	fibril_mutex_unlock(&job->state_mtx);

	switch (job->type) {
	case JOB_START:
		retval = job_run_start(job);
		break;
	default:
		assert(false);
	}


finish:
	fibril_mutex_lock(&job->state_mtx);
	job->state = JOB_FINISHED;
	job->retval = retval;
	fibril_condvar_broadcast(&job->state_cv);
	fibril_mutex_unlock(&job->state_mtx);

	job_del_ref(&job);

	return EOK;
}

static int job_dispatcher(void *arg)
{
	fibril_mutex_lock(&job_queue_mtx);
	while (1) {
		while (list_empty(&job_queue)) {
			fibril_condvar_wait(&job_queue_cv, &job_queue_mtx);
		}
		
		link_t *link = list_first(&job_queue);
		assert(link);
		list_remove(link);

		/*
		 * Note that possible use of fibril pool must hold invariant
		 * that job is started asynchronously. In the case there exists
		 * circular dependency between jobs, it may result in a deadlock.
		 */
		job_t *job = list_get_instance(link, job_t, link);
		fid_t runner_fibril = fibril_create(job_runner, job);
		fibril_add_ready(runner_fibril);
	}

	fibril_mutex_unlock(&job_queue_mtx);
	return EOK;
}

void job_queue_init()
{
	list_initialize(&job_queue);
	fibril_mutex_initialize(&job_queue_mtx);
	fibril_condvar_initialize(&job_queue_cv);

	fid_t dispatcher_fibril = fibril_create(job_dispatcher, NULL);
	fibril_add_ready(dispatcher_fibril);
}

int job_queue_jobs(list_t *jobs)
{
	fibril_mutex_lock(&job_queue_mtx);

	/* Check consistency with queue. */
	list_foreach(*jobs, link, job_t, new_job) {
		list_foreach(job_queue, link, job_t, queued_job) {
			/*
			 * Currently we have strict strategy not permitting
			 * multiple jobs for one unit in the queue.
			 */
			if (new_job->unit == queued_job->unit) {
				return EEXIST;
			}
		}
	}

	/* Enqueue jobs */
	list_foreach_safe(*jobs, cur_link, next_lin) {
		list_remove(cur_link);
		list_append(cur_link, &job_queue);
	}

	/* Only job dispatcher waits, it's correct to notify one only. */
	fibril_condvar_signal(&job_queue_cv);
	fibril_mutex_unlock(&job_queue_mtx);

	return EOK;
}

/** Blocking wait for job finishing.
 *
 * Multiple fibrils may wait for the same job.
 *
 * @return    Return code of the job
 */
int job_wait(job_t *job)
{
	fibril_mutex_lock(&job->state_mtx);
	while (job->state != JOB_FINISHED) {
		fibril_condvar_wait(&job->state_cv, &job->state_mtx);
	}

	int rc = job->retval;
	fibril_mutex_unlock(&job->state_mtx);

	return rc;
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

static void job_init(job_t *job, job_type_t type)
{
	assert(job);

	link_initialize(&job->link);
	list_initialize(&job->blocking_jobs);

	/* Start with one reference for the creator */
	atomic_set(&job->refcnt, 1);

	job->type = type;
	job->unit = NULL;

	job->state = JOB_WAITING;
	fibril_mutex_initialize(&job->state_mtx);
	fibril_condvar_initialize(&job->state_cv);
}

job_t *job_create(job_type_t type)
{
	job_t *job = malloc(sizeof(job_t));
	if (job != NULL) {
		job_init(job, type);
	}

	return job;
}

static void job_destroy(job_t **job_ptr)
{
	job_t *job = *job_ptr;
	if (job == NULL) {
		return;
	}

	list_foreach_safe(job->blocking_jobs, cur_link, next_link) {
		job_link_t *jl = list_get_instance(cur_link, job_link_t, link);
		list_remove(cur_link);
		job_del_ref(&jl->job);
		free(jl);
	}
	free(job);

	*job_ptr = NULL;
}
