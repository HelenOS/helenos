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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "../tester.h"
#include "common.h"


const char *test_proc_task_wait(void)
{
	const char *err = NULL;

	int rc;
	task_id_t tid;
	task_wait_t wait;
	int retval;
	task_exit_t texit;

	TPRINTF("11 match\n");

	task_wait_set(&wait, TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_FAIL);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	TPRINTF("OK\n");
	/* ---- */
	
	TPRINTF("12 lost wait\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_FAIL);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EINVAL);
	TASSERT(task_wait_get(&wait) == 0);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("13 partial match\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL | TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_BYPASS);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	/* retval is undefined */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("21 ignore retval and still wait for exit\n");

	task_wait_set(&wait, TASK_WAIT_EXIT);
	/* STR_JOB_OK to emulate daemon that eventually terminates */
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_NORMAL);
	/* retval is unknown */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("22 good match\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_DAEMON);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	/* exit is not expected */
	TASSERT(retval == EOK);
	task_kill(tid); /* Terminate daemon */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("23 partial match (non-exited task)\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL | TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_DAEMON);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == TASK_WAIT_EXIT);
	/* exit is not expected */
	TASSERT(retval == EOK);
	task_kill(tid); /* Terminate daemon */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("31 on exit return\n");

	task_wait_set(&wait, TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_NORMAL);
	/* retval is unknown */
	TPRINTF("OK\n");
	/* ---- */


	TPRINTF("32 keep retval until exit\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	/* Job atomically exited, so there's nothing more to wait for. */
	TASSERT(task_wait_get(&wait) == 0);
	/* exit is unknown */
	TASSERT(retval == EOK);
	/* check task already exited */
	rc = task_kill(tid);
	TASSERT(rc == ENOENT);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("33 double good match\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL | TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_NORMAL);
	TASSERT(retval == EOK);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("14 partially lost wait\n");

	task_wait_set(&wait, TASK_WAIT_BOTH);
	rc = dummy_task_spawn(&tid, &wait, STR_FAIL);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EINVAL);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	/* retval is undefined */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("24 repeated wait\n");

	task_wait_set(&wait, TASK_WAIT_BOTH);
	rc = dummy_task_spawn(&tid, &wait, STR_DAEMON);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == TASK_WAIT_EXIT);
	TASSERT(retval == EOK);
	task_kill(tid); /* Terminate daemon */
	TPRINTF("waiting 2...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("34 double wait in one\n");

	task_wait_set(&wait, TASK_WAIT_BOTH);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	texit = TASK_EXIT_RUNNING; retval = 255;
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(task_wait_get(&wait) == 0);
	TASSERT(texit == TASK_EXIT_NORMAL);
	TASSERT(retval == EOK);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("All task waiting tests finished");



	return err;
}
