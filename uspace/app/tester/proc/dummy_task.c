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
#include <stdlib.h>
#include <str.h>
#include <task.h>

#include "../tester.h"
#include "common.h"

typedef void (* behavior_func_t)(void);

typedef struct {
	const char *name;
	task_behavior_t behavior;
	behavior_func_t func;
} behavior_item_t;

static const char *err = NULL;

static void dummy_fail(void)
{
	task_id_t id = task_get_id();
	printf("Gonna shoot myself (%" PRIu64 ").\n", id);
	behavior_func_t func = NULL;
	func();
}

static void dummy_bypass(void)
{
	__SYSCALL1(SYS_TASK_EXIT, false);
}

static void dummy_daemon(void)
{
	task_retval(EOK);
	async_manager();
}

static void dummy_job_fail(void)
{
	err = "Intended error";
}

static void dummy_job_ok(void)
{
	err = NULL;
}

static behavior_item_t behaviors[] = {
	{ STR_FAIL,     BEHAVIOR_FAIL,     &dummy_fail },
	{ STR_BYPASS,   BEHAVIOR_BYPASS,   &dummy_bypass },
	{ STR_DAEMON,   BEHAVIOR_DAEMON,   &dummy_daemon },
	{ STR_JOB_FAIL, BEHAVIOR_JOB_FAIL, &dummy_job_fail },
	{ STR_JOB_OK,   BEHAVIOR_JOB_OK,   &dummy_job_ok },
	{ NULL,         BEHAVIOR_JOB_OK,   NULL }
};

const char *test_proc_dummy_task(void)
{
	const char *name = (test_argc == 0) ? STR_JOB_OK : test_argv[0];

	for (behavior_item_t *it = behaviors; it->name != NULL; ++it) {
		if (str_cmp(name, it->name) != 0) {
			continue;
		}
		it->func();
	}

	return err;
}
