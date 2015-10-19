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

#ifndef PROC_COMMON_H_
#define PROC_COMMON_H_

typedef enum {
	BEHAVIOR_FAIL,
	BEHAVIOR_BYPASS,
	BEHAVIOR_DAEMON,
	BEHAVIOR_JOB_FAIL,
	BEHAVIOR_JOB_OK
} task_behavior_t;

#define STR_FAIL        "fail"
#define STR_BYPASS      "bypass"
#define STR_DAEMON      "daemon"
#define STR_JOB_FAIL    "job-fail"
#define STR_JOB_OK      "job-ok"

#define DUMMY_TASK     "/root/app/tester"
#define DUMMY_TASK_ARG "proc_dummy_task"

static inline int dummy_task_spawn(task_id_t *task_id, task_wait_t *wait,
    const char *behavior)
{
	return task_spawnl(task_id, wait,
	    DUMMY_TASK, DUMMY_TASK, DUMMY_TASK_ARG, behavior,
	    NULL);
}

#endif
