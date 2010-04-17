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
#include <sysinfo.h>

#include "func.h"

#define TASK_COUNT 10
#define THREAD_COUNT 50

#define ECHOLOAD1(x) ((x) >> 11)
#define ECHOLOAD2(x) (((x) & 0x7ff) / 2)

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

	printf("      ID  Threads      Mem       uCycles       kCycles   Cycle fault Name\n");

	int i;
	for (i = 0; i < result; ++i) {
		task_info_t taskinfo;
		get_task_info(tasks[i], &taskinfo);
		uint64_t mem, ucycles, kcycles;
		char memsuffix, usuffix, ksuffix;
		order(taskinfo.virt_mem, &mem, &memsuffix);
		order(taskinfo.ucycles, &ucycles, &usuffix);
		order(taskinfo.kcycles, &kcycles, &ksuffix);
		printf("%8llu %8u %8llu%c %12llu%c %12llu%c %s\n", tasks[i],
			taskinfo.thread_count, mem, memsuffix, ucycles, usuffix,
			kcycles, ksuffix, taskinfo.name);
	}

	free(tasks);
}

static void list_threads(task_id_t taskid)
{
	size_t thread_count = THREAD_COUNT;
	thread_info_t *threads = malloc(thread_count * sizeof(thread_info_t));
	size_t result = get_task_threads(threads, sizeof(thread_info_t) * thread_count);

	while (result > thread_count) {
		thread_count *= 2;
		threads = realloc(threads, thread_count * sizeof(thread_info_t));
		result = get_task_threads(threads, sizeof(thread_info_t) * thread_count);
	}

	if (result == 0) {
		printf("No task with given pid!\n");
		exit(1);
	}

	size_t i;
	printf("    ID    State  CPU   Prio    [k]uCycles    [k]kcycles   Cycle fault\n");
	for (i = 0; i < result; ++i) {
		if (threads[i].taskid != taskid) {
			continue;
		}
		uint64_t ucycles, kcycles;
		char usuffix, ksuffix;
		order(threads[i].ucycles, &ucycles, &usuffix);
		order(threads[i].kcycles, &kcycles, &ksuffix);
		printf("%6llu %-8s %4u %6d %12llu%c %12llu%c\n", threads[i].tid,
			thread_states[threads[i].state], threads[i].cpu,
			threads[i].priority, ucycles, usuffix,
			kcycles, ksuffix);
	}

	free(threads);
}

static void echo_load(void)
{
	unsigned long load[3];
	get_load(load);
	printf("load avarage: ");
	print_load_fragment(load[0], 2);
	puts(" ");
	print_load_fragment(load[1], 2);
	puts(" ");
	print_load_fragment(load[2], 2);
	puts("\n");
}

static void echo_cpus(void)
{
	size_t cpu_count = sysinfo_value("cpu.count");
	printf("Found %u cpu's:\n", cpu_count);
	uspace_cpu_info_t *cpus = malloc(cpu_count * sizeof(uspace_cpu_info_t));
	get_cpu_info(cpus);
	size_t i;
	for (i = 0; i < cpu_count; ++i) {
		printf("%2u (%4u Mhz): Busy ticks: %6llu, Idle ticks: %6llu\n", cpus[i].id,
				(size_t)cpus[i].frequency_mhz, cpus[i].busy_ticks, cpus[i].idle_ticks);
	}
}

static void usage()
{
	printf("Usage: ps [-t pid|-l|-c]\n");
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
		} else if (str_cmp(*argv, "-l") == 0) {
			--argc; ++argv;
			if (argc != 0) {
				printf("Bad argument count!\n");
				usage();
				exit(1);
			}
			echo_load();
		} else if (str_cmp(*argv, "-c") == 0) {
			--argc; ++argv;
			if (argc != 0) {
				printf("Bad argument count!\n");
				usage();
				exit(1);
			}
			echo_cpus();
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
