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
#include "job_closure.h"
#include "log.h"


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


/*
 * Non-static functions
 */
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

