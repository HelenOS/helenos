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

/** @addtogroup ps
 * @brief Task lister.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <task.h>
#include <thread.h>
#include <ps.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <load.h>

#include "func.h"

#define TASK_COUNT 10
#define THREAD_COUNT 50

/** Thread states */
static const char *thread_states[] = {
	"Invalid",
	"Running",
	"Sleeping",
	"Ready",
	"Entering",
	"Exiting",
	"Lingering"
}; 

static void list_tasks(void)
{
	int task_count = TASK_COUNT;
	task_id_t *tasks = malloc(task_count * sizeof(task_id_t));
	int result = get_task_ids(tasks, sizeof(task_id_t) * task_count);

	while (result > task_count) {
		task_count *= 2;
		tasks = realloc(tasks, task_count * sizeof(task_id_t));
		result = get_task_ids(tasks, sizeof(task_id_t) * task_count);
	}

	printf("      ID  Threads    Pages       uCycles       kCycles   Cycle fault Name\n");

	int i;
	for (i = 0; i < result; ++i) {
		task_info_t taskinfo;
		get_task_info(tasks[i], &taskinfo);
		uint64_t ucycles, kcycles, fault;
		char usuffix, ksuffix, fsuffix;
		order(taskinfo.ucycles, &ucycles, &usuffix);
		order(taskinfo.kcycles, &kcycles, &ksuffix);
		order((taskinfo.kcycles + taskinfo.ucycles) - taskinfo.cycles, &fault, &fsuffix);
		printf("%8llu %8u %8u %12llu%c %12llu%c %12llu%c %s\n", tasks[i],
			taskinfo.thread_count, taskinfo.pages, ucycles, usuffix,
			kcycles, ksuffix, fault, fsuffix, taskinfo.name);
	}
}

static void list_threads(task_id_t taskid)
{
	int thread_count = THREAD_COUNT;
	thread_info_t *threads = malloc(thread_count * sizeof(thread_info_t));
	int result = get_task_threads(taskid, threads, sizeof(thread_info_t) * thread_count);

	while (result > thread_count) {
		thread_count *= 2;
		threads = realloc(threads, thread_count * sizeof(thread_info_t));
		result = get_task_threads(taskid, threads, sizeof(thread_info_t) * thread_count);
	}

	if (result == 0) {
		printf("No task with given pid!\n");
		exit(1);
	}

	int i;
	printf("    ID    State  CPU   Prio    [k]uCycles    [k]kcycles   Cycle fault\n");
	for (i = 0; i < result; ++i) {
		uint64_t ucycles, kcycles, fault;
		char usuffix, ksuffix, fsuffix;
		order(threads[i].ucycles, &ucycles, &usuffix);
		order(threads[i].kcycles, &kcycles, &ksuffix);
		order((threads[i].kcycles + threads[i].ucycles) - threads[i].cycles, &fault, &fsuffix);
		printf("%6llu %-8s %4u %6d %12llu%c %12llu%c %12llu%c\n", threads[i].tid,
			thread_states[threads[i].state], threads[i].cpu,
			threads[i].priority, ucycles, usuffix,
			kcycles, ksuffix, fault, fsuffix);
	}
}

static void echo_load(void)
{
	size_t load[3];
	load[0] = 0;
	load[1] = 0;
	load[2] = 0;
	get_load(load);
	printf("Current load: %d.%03d %d.%03d %d.%03d\n", load[0] >> 11, (load[0] & 0x7ff) / 2, load[1] >> 11, (load[1] & 0x7ff) / 2, load[2] >> 11, (load[2] & 0x7ff) / 2);
}

static void usage()
{
	printf("Usage: ps [-t pid -l]\n");
}

int main(int argc, char *argv[])
{
	--argc; ++argv;

	if (argc > 0)
	{
		if (str_cmp(*argv, "-t") == 0) {
			--argc; ++argv;
			if (argc != 1) {
				printf("Bad argument count!\n");
				usage();
				exit(1);
			}
			task_id_t taskid = strtol(*argv, NULL, 10);
			list_threads(taskid);
		} if (str_cmp(*argv, "-l") == 0) {
			--argc; ++argv;
			if (argc != 0) {
				printf("Bad argument count!\n");
				usage();
				exit(1);
			}
			echo_load();
		} else {
			printf("Unknown argument %s!\n", *argv);
			usage();
			exit(1);
		}
	} else {
		list_tasks();
	}

	return 0;
}

/** @}
 */
