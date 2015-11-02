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
#include <stdlib.h>
#include <task.h>

#include "configuration.h"
#include "log.h"
#include "sysman.h"
#include "sm_task.h"

/** Structure for boxing task event */
struct sm_task_event {
	task_id_t task_id;
	int flags;
	task_exit_t texit;
	int retval;
};

static void sysman_event_task_event(void *);

/**
 * @note This function runs in separate fibril (not same as event loop).
 */
static void sm_task_event_handler(task_id_t tid, int flags, task_exit_t texit,
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
	list_foreach(units, units, unit_t, u) {
		if (u->type != UNIT_SERVICE) {
			continue;
		}
		if (CAST_SVC(u)->main_task_id == tid) {
			return CAST_SVC(u);
		}

	}

	return NULL;
}

static void sysman_event_task_event(void *data)
{
	sm_task_event_t *tev = data;

	sysman_log(LVL_DEBUG2, "%s, %" PRIu64 " %i",
	    __func__, tev->task_id, tev->flags);
	unit_svc_t *u_svc = sm_task_find_service(tev->task_id);
	if (u_svc == NULL) {
		goto finish;
	}


	/* Simple incomplete state automaton */
	unit_t *u = &u_svc->unit;
	sysman_log(LVL_DEBUG2, "%s, %s(%i)@%" PRIu64 " %i",
	    __func__, unit_name(u), u->state, tev->task_id, tev->flags);
	assert(u->state == STATE_STARTING);

	if (tev->flags & TASK_WAIT_EXIT) {
		// TODO maybe call unit_fail (would be nice to contain reason)
		u->state = STATE_FAILED;
	} else {
		u->state = STATE_STARTED;
	}

	unit_notify_state(u);

finish:
	free(tev);
}

int sm_task_init(void)
{
	int rc = task_register_event_handler(&sm_task_event_handler);

	//TODO dump taskman info for boot time tasks
	return rc;
}
