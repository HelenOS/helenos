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

/** @addtogroup tasks
 * @brief Task lister.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <task.h>
#include <thread.h>
#include <stats.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <inttypes.h>
#include <arg_parse.h>
#include "func.h"

#define NAME  "tasks"

#define TASK_COUNT    10
#define THREAD_COUNT  50

#define PRINT_LOAD1(x)  ((x) >> 11)
#define PRINT_LOAD2(x)  (((x) & 0x7ff) / 2)

/** Thread states */
//static const char *thread_states[] = {
//	"Invalid",
//	"Running",
//	"Sleeping",
//	"Ready",
//	"Entering",
//	"Exiting",
//	"Lingering"
//};

static void list_tasks(void)
{
	size_t count;
	task_id_t *ids =
	    (task_id_t *) stats_get_tasks(&count);
	
	if (ids == NULL) {
		fprintf(stderr, "%s: Unable to get tasks\n", NAME);
		return;
	}
	
	printf("      ID  Threads      Mem       uCycles       kCycles   Name\n");
	
	size_t i;
	for (i = 0; i < count; i++) {
		stats_task_t *stats_task = stats_get_task(ids[i]);
		if (stats_task != NULL) {
			uint64_t virtmem, ucycles, kcycles;
			char vmsuffix, usuffix, ksuffix;
			
			order(stats_task->virtmem, &virtmem, &vmsuffix);
			order(stats_task->ucycles, &ucycles, &usuffix);
			order(stats_task->kcycles, &kcycles, &ksuffix);
			
			printf("%8" PRIu64 "%8u %8" PRIu64"%c %12"
			    PRIu64"%c %12" PRIu64"%c %s\n", ids[i], stats_task->threads,
			    virtmem, vmsuffix, ucycles, usuffix, kcycles, ksuffix,
			    stats_task->name);
			
			free(stats_task);
		} else
			printf("%8" PRIu64 "\n", ids[i]);
	}
	
	free(ids);
}

static void list_threads(task_id_t task_id)
{
	/* TODO:
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

	free(threads); */
}

static void print_load(void)
{
	size_t count;
	load_t *load = stats_get_load(&count);
	
	if (load == NULL) {
		fprintf(stderr, "%s: Unable to get load\n", NAME);
		return;
	}
	
	printf("%s: Load avarage: ", NAME);
	
	size_t i;
	for (i = 0; i < count; i++) {
		if (i > 0)
			printf(" ");
		
		stats_print_load_fragment(load[i], 2);
	}
	
	printf("\n");
	
	free(load);
}

static void list_cpus(void)
{
	size_t count;
	stats_cpu_t *cpus = stats_get_cpus(&count);
	
	if (cpus == NULL) {
		fprintf(stderr, "%s: Unable to get CPU statistics\n", NAME);
		return;
	}
	
	printf("%s: %u CPU(s) detected\n", NAME, count);
	
	size_t i;
	for (i = 0; i < count; i++) {
		printf("cpu%u: %" PRIu16 " MHz, busy ticks: "
		    "%" PRIu64 ", idle ticks: %" PRIu64 "\n",
		    cpus[i].id, cpus[i].frequency_mhz, cpus[i].busy_ticks,
		    cpus[i].idle_ticks);
	}
	
	free(cpus);
}

static void usage()
{
	printf(
	    "Usage: tasks [-t task_id] [-l] [-c]\n" \
	    "\n" \
	    "Options:\n" \
	    "\t-t task_id\n" \
	    "\t--task=task_id\n" \
	    "\t\tList threads of the given task\n" \
	    "\n" \
	    "\t-l\n" \
	    "\t--load\n" \
	    "\t\tPrint system load\n" \
	    "\n" \
	    "\t-c\n" \
	    "\t--cpu\n" \
	    "\t\tList CPUs\n" \
	    "\n" \
	    "\t-h\n" \
	    "\t--help\n" \
	    "\t\tPrint this usage information\n"
	    "\n" \
	    "Without any options all tasks are listed\n"
	);
}

int main(int argc, char *argv[])
{
	bool toggle_tasks = true;
	bool toggle_threads = false;
	bool toggle_load = false;
	bool toggle_cpus = false;
	
	task_id_t task_id;
	
	int i;
	for (i = 1; i < argc; i++) {
		int off;
		
		/* Usage */
		if ((off = arg_parse_short_long(argv[i], "-h", "--help")) != -1) {
			usage();
			return 0;
		}
		
		/* Load */
		if ((off = arg_parse_short_long(argv[i], "-l", "--load")) != -1) {
			toggle_tasks = false;
			toggle_load = true;
			continue;
		}
		
		/* CPUs */
		if ((off = arg_parse_short_long(argv[i], "-c", "--cpus")) != -1) {
			toggle_tasks = false;
			toggle_cpus = true;
			continue;
		}
		
		/* Threads */
		if ((off = arg_parse_short_long(argv[i], "-t", "--task=")) != -1) {
			// TODO: Support for 64b range
			int tmp;
			int ret = arg_parse_int(argc, argv, &i, &tmp, off);
			if (ret != EOK) {
				printf("%s: Malformed task_id '%s'\n", NAME, argv[i]);
				return -1;
			}
			
			task_id = tmp;
			
			toggle_tasks = false;
			toggle_threads = true;
			continue;
		}
	}
	
	if (toggle_tasks)
		list_tasks();
	
	if (toggle_threads)
		list_threads(task_id);
	
	if (toggle_load)
		print_load();
	
	if (toggle_cpus)
		list_cpus();
	
	return 0;
}

/** @}
 */
