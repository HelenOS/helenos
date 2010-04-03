/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup top
 * @brief Task lister.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <task.h>
#include <thread.h>
#include <stdlib.h>
#include <malloc.h>
#include <ps.h>
#include <sysinfo.h>
#include "ps.h"

#define TASK_COUNT 10
#define THREAD_COUNT 50

/** Thread states */
const char *thread_states[] = {
	"Invalid",
	"Running",
	"Sleeping",
	"Ready",
	"Entering",
	"Exiting",
	"Lingering"
}; 

unsigned int get_tasks(task_id_t **out_tasks)
{
	int task_count = TASK_COUNT;
	task_id_t *tasks = malloc(task_count * sizeof(task_id_t));
	int result = get_task_ids(tasks, sizeof(task_id_t) * task_count);

	while (result > task_count) {
		task_count *= 2;
		tasks = realloc(tasks, task_count * sizeof(task_id_t));
		result = get_task_ids(tasks, sizeof(task_id_t) * task_count);
	}

/*
	int i;
	for (i = 0; i < result; ++i) {
		task_info_t taskinfo;
		get_task_info(tasks[i], &taskinfo);
	}
	*/

	*out_tasks = tasks;
	return result;
}

thread_info_t *get_threads(task_id_t taskid)
{
	int thread_count = THREAD_COUNT;
	thread_info_t *threads = malloc(thread_count * sizeof(thread_info_t));
	int result = get_task_threads(taskid, threads, sizeof(thread_info_t) * thread_count);

	while (result > thread_count) {
		thread_count *= 2;
		threads = realloc(threads, thread_count * sizeof(thread_info_t));
		result = get_task_threads(taskid, threads, sizeof(thread_info_t) * thread_count);
	}
	
	return threads;
}

unsigned int get_cpu_infos(uspace_cpu_info_t **out_infos)
{
	unsigned int cpu_count = sysinfo_value("cpu.count");
	uspace_cpu_info_t *cpus = malloc(cpu_count * sizeof(uspace_cpu_info_t));
	get_cpu_info(cpus);

	*out_infos = cpus;
	return cpu_count;
}

/** @}
 */
