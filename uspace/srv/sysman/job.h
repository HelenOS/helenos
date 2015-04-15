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

#include <adt/list.h>
#include <atomic.h>

#include "unit.h"

struct job;
typedef struct job job_t;

typedef enum {
	JOB_START
} job_type_t;

typedef enum {
	JOB_WAITING,
	JOB_RUNNING,
	JOB_FINISHED
} job_state_t;

typedef struct {
	link_t link;
	job_t *job;
} job_link_t;

/** Job represents pending or running operation on unit */
struct job {
	/** Link to queue job is in */
	link_t link;

	/** List of jobs (job_link_t ) that are blocking the job. */
	list_t blocking_jobs;

	/** Reference counter for the job structure. */
	atomic_t refcnt;

	job_type_t type;
	unit_t *unit;

	job_state_t state;
	fibril_mutex_t state_mtx;
	fibril_condvar_t state_cv;

	/** Return value of the job, defined only when state == JOB_FINISHED */
	int retval;
};

extern void job_queue_init(void);
extern int job_queue_jobs(list_t *);

extern int job_wait(job_t *);

extern void job_add_ref(job_t *);
extern void job_del_ref(job_t **);

extern job_t *job_create(job_type_t type);
extern int job_add_blocking_job(job_t *, job_t *);

#endif
