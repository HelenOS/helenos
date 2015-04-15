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

	list_foreach(unit->dependencies, dependencies, unit_dependency_t, dep) {
		job_t *blocking_job = NULL;
		rc = sysman_create_closure_jobs(dep->dependency, &blocking_job,
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
	// TODO shouldn't be here read-lock on configuration?
	int rc = sysman_create_closure_jobs(unit, &job, &new_jobs, JOB_START);
	if (rc != EOK) {
		return rc;
	}

	// TODO handle errors when adding job accumulator
	job_queue_jobs(&new_jobs);

	return job_wait(job);
}
