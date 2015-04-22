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

#ifndef SYSMAN_JOB_H
#define SYSMAN_JOB_H

#include <adt/dyn_array.h>
#include <adt/list.h>
#include <atomic.h>
#include <stdbool.h>

#include "unit.h"

/** Run state of job */
typedef enum {
	JOB_UNQUEUED, /**< Job not in queue yet */
	JOB_QUEUED,
	JOB_DEQUEUED, /**< Job not in queue already */
	JOB_RUNNING,
	JOB_FINISHED
} job_state_t;

/** Return value of job */
typedef enum {
	JOB_OK,
	JOB_FAILED,
	JOB_UNDEFINED_ = -1
} job_retval_t;

typedef struct {
	link_t job_queue;
	atomic_t refcnt;

	unit_state_t target_state;
	unit_t *unit;

	/** Jobs that this job is preventing from running */
	dyn_array_t blocked_jobs;
	/** No. of jobs that must finish before this job */
	size_t blocking_jobs;
	/** Any of blocking jobs failed */
	bool blocking_job_failed;

	/** See job_state_t */
	job_state_t state;
	/** See job_retval_t */
	job_retval_t retval;
} job_t;

typedef job_t *job_ptr_t;

extern void job_queue_init(void);
extern int job_queue_add_jobs(dyn_array_t *);
extern job_t *job_queue_pop_runnable(void);

extern int job_create_closure(job_t *, dyn_array_t *);
extern job_t *job_create(unit_t *, unit_state_t);

extern void job_add_ref(job_t *);
extern void job_del_ref(job_t **);


extern void job_run(job_t *);
extern void job_finish(job_t *);
#endif
