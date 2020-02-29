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
#include <stdlib.h>
#include <task.h>
#include <sysman/unit.h>

#include "repo.h"
#include "log.h"
#include "sysman.h"
#include "sm_task.h"

/** Structure for boxing task event */
struct sm_task_event {
	task_id_t task_id;
	task_wait_flag_t flags;
	task_exit_t texit;
	int retval;
};

static void sysman_event_task_event(void *);

/**
 * @note This function runs in separate fibril (not same as event loop).
 */
static void sm_task_event_handler(task_id_t tid, task_wait_flag_t flags, task_exit_t texit,
    int retval)
{
	sm_task_event_t *tev = malloc(sizeof(sm_task_event_t));
	if (tev == NULL) {
		sysman_log(LVL_FATAL,
		    "Unable to process event of task %" PRIu64 ".", tid);
		return;
	}
	tev->task_id = tid;
	tev->flags = flags;
	tev->texit = texit;
	tev->retval = retval;

	sysman_raise_event(&sysman_event_task_event, tev);
}

static unit_svc_t *sm_task_find_service(task_id_t tid)
{
	/*
	 * Unit to task is about to be developed, so use plain linear search
	 * instead of specialized structures.
	 */
	repo_foreach_t(UNIT_SERVICE, u) {
		if (CAST_SVC(u)->main_task_id == tid) {
			return CAST_SVC(u);
		}

	}

	return NULL;
}

static unit_svc_t *sm_task_create_service(task_id_t tid)
{
	unit_t *u_svc = unit_create(UNIT_SERVICE);
	bool in_repo_update = false;
	errno_t rc = EOK;

	if (u_svc == NULL) {
		goto fail;
	}

	rc = asprintf(&u_svc->name, ANONYMOUS_SERVICE_MASK "%c%s", tid,
	    UNIT_NAME_SEPARATOR, UNIT_SVC_TYPE_NAME);
	if (rc < 0) {
		goto fail;
	}

	CAST_SVC(u_svc)->main_task_id = tid;
	CAST_SVC(u_svc)->anonymous = true;
	/*
	 * exec_start is left undefined, maybe could be hinted by kernel's task
	 * name
	 */

	/*
	 * Temporary workaround to avoid killing ourselves during shutdown,
	 * eventually should be captured by dependencies.
	 */
	if (tid == task_get_id() || tid == 2 /*taskman*/) {
		CAST_SVC(u_svc)->critical = true;
	}

	repo_begin_update();
	in_repo_update = true;

	rc = repo_add_unit(u_svc);
	if (rc != EOK) {
		goto fail;
	}

	repo_commit();

	return CAST_SVC(u_svc);

fail:
	if (in_repo_update) {
		repo_rollback();
	}

	unit_destroy(&u_svc);
	return NULL;
}

static void sm_task_delete_service(unit_svc_t *u_svc)
{
	repo_begin_update();
	errno_t rc = repo_remove_unit(&u_svc->unit);
	if (rc != EOK) {
		sysman_log(LVL_WARN, "Can't remove unit %s (%i).",
		    unit_name(&u_svc->unit), rc);
		repo_rollback();
		return;
	}

	repo_commit();
}

static void sysman_event_task_event(void *data)
{
	sm_task_event_t *tev = data;

	sysman_log(LVL_DEBUG2, "%s, %" PRIu64 " %i",
	    __func__, tev->task_id, tev->flags);
	unit_svc_t *u_svc = sm_task_find_service(tev->task_id);

	if (u_svc == NULL) {
		if (tev->flags & TASK_WAIT_EXIT) {
			/* Non-service task exited, ignore. */
			goto finish;
		}

		u_svc = sm_task_create_service(tev->task_id);
		if (u_svc == NULL) {
			sysman_log(LVL_WARN,
			    "Unable to create anonymous service for task %" PRIu64 ".",
			    tev->task_id);
			goto finish;
		}

		sysman_log(LVL_DEBUG, "Created anonymous service %s.",
		    unit_name(&u_svc->unit));

		/* Inject state so that further processing makes sense */
		u_svc->unit.state = STATE_STARTING;
	}

	/* Simple incomplete state automaton */
	unit_t *u = &u_svc->unit;
	sysman_log(LVL_DEBUG2, "%s, %s(%i)@%" PRIu64 " %i",
	    __func__, unit_name(u), u->state, tev->task_id, tev->flags);

	if (tev->flags & TASK_WAIT_EXIT) {
		// TODO maybe call unit_fail (would be nice to contain reason)
		//      or move this whole logic to unit_svc.c
		if (u->state == STATE_STOPPING) {
			u->state = STATE_STOPPED;
		} else {
			// if it has also retval == 0 then it's not fail
			u->state = STATE_FAILED;
		}

	} else if (tev->flags & TASK_WAIT_RETVAL) {
		assert(u->state == STATE_STARTING);
		u->state = STATE_STARTED;
	}

	unit_notify_state(u);

	if ((tev->flags & TASK_WAIT_EXIT) && u_svc->anonymous) {
		sysman_log(LVL_DEBUG, "Deleted anonymous service %s.",
		    unit_name(&u_svc->unit));
		sm_task_delete_service(u_svc);
	}
finish:
	free(tev);
}

errno_t sm_task_start(void)
{
	return task_register_event_handler(&sm_task_event_handler, true);
}
