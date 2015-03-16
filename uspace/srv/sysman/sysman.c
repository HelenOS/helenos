#include <adt/list.h>
#include <errno.h>

#include "dep.h"
#include "job.h"
#include "sysman.h"

/** Create jobs for cluser of given unit.
 *
 * @note Using recursion, limits "depth" of dependency graph.
 */
static int sysman_create_closure_jobs(unit_t *unit, job_t **entry_job_ptr,
    list_t *accumulator, job_type_t type)
{
	int rc = EOK;
	job_t *job = job_create(type);
	if (job == NULL) {
		rc = ENOMEM;
		goto fail;
	}

	job->unit = unit;

	list_foreach(unit->dependencies, dependencies, unit_dependency_t, edge) {
		job_t *blocking_job = NULL;
		rc = sysman_create_closure_jobs(edge->dependency, &blocking_job,
		    accumulator, type);
		if (rc != EOK) {
			goto fail;
		}
		
		rc = job_add_blocking_job(job, blocking_job);
		if (rc != EOK) {
			goto fail;
		}
	}

	/* Job is passed to the accumulator, i.e. no add_ref. */
	list_append(&job->link, accumulator);

	if (entry_job_ptr != NULL) {
		*entry_job_ptr = job;
	}
	return EOK;

fail:
	job_del_ref(&job);
	return rc;
}

int sysman_unit_start(unit_t *unit)
{
	list_t new_jobs;
	list_initialize(&new_jobs);

	job_t *job = NULL;
	int rc = sysman_create_closure_jobs(unit, &job, &new_jobs, JOB_START);
	if (rc != EOK) {
		return rc;
	}

	// TODO handle errors when adding job accumulator
	job_queue_jobs(&new_jobs);

	return job_wait(job);
}
