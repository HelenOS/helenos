#ifndef SYSMAN_JOB_H
#define SYSMAN_JOB_H

#include <adt/list.h>

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

struct job {
	link_t link;

	list_t blocking_jobs;

	job_type_t type;
	unit_t *unit;

	job_state_t state;
	fibril_mutex_t state_mtx;
	fibril_condvar_t state_cv;

	int retval;
};

extern void job_queue_init(void);
extern int job_queue_jobs(list_t *);

extern int job_wait(job_t *);

extern job_t *job_create(job_type_t type);
extern void job_destroy(job_t **);

#endif
