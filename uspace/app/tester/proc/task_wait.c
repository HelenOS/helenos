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

#define DUMMY_TASK     "/root/app/tester"
#define DUMMY_TASK_ARG "proc_dummy_task"

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

#define TASSERT(expr) \
	do { \
		if (!(expr)) \
			return "Failed " #expr " " __FILE__ ":" S__LINE__ ; \
	} while (0)

static int dummy_task_spawn(task_id_t *task_id, task_wait_t *wait,
    const char *behavior)
{
	return task_spawnl(task_id, wait,
	    DUMMY_TASK, DUMMY_TASK, DUMMY_TASK_ARG, behavior,
	    NULL);
}

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
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	TPRINTF("OK\n");
	/* ---- */
	
	TPRINTF("12 lost wait\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_FAIL);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EINVAL);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("13 partial match\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL | TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_BYPASS);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(texit == TASK_EXIT_UNEXPECTED);
	/* retval is undefined */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("21 ignore retval\n");

	task_wait_set(&wait, TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(texit == TASK_EXIT_NORMAL);
	/* retval is unknown */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("22 good match\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_DAEMON);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	/* exit is not expected */
	TASSERT(retval == EOK);
	task_kill(tid); /* Terminate daemon */
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("23 partial match (non-exited task)\n");

	// TODO should update wait for synchronized exit waiting
	task_wait_set(&wait, TASK_WAIT_RETVAL | TASK_WAIT_EXIT);
	rc = dummy_task_spawn(&tid, &wait, STR_DAEMON);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
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
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(texit == TASK_EXIT_NORMAL);
	/* retval is unknown */
	TPRINTF("OK\n");
	/* ---- */


	TPRINTF("32 keep retval until exit\n");

	task_wait_set(&wait, TASK_WAIT_RETVAL);
	rc = dummy_task_spawn(&tid, &wait, STR_JOB_OK);
	TASSERT(rc == EOK);

	TPRINTF("waiting...");
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
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
	rc = task_wait(&wait, &texit, &retval);
	TPRINTF("done.\n");
	TASSERT(rc == EOK);
	TASSERT(texit == TASK_EXIT_NORMAL);
	TASSERT(retval == EOK);
	TPRINTF("OK\n");
	/* ---- */

	TPRINTF("All task waiting tests finished");



	return err;
}
