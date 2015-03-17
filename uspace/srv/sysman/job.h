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
