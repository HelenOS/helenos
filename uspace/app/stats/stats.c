/*
 * Copyright (c) 2010 Stanislav Kozina
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup stats
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <task.h>
#include <stats.h>
#include <errno.h>
#include <stdlib.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <str.h>
#include <arg_parse.h>

#define NAME  "stats"

#define DAY     86400
#define HOUR    3600
#define MINUTE  60

#define KERNEL_NAME  "kernel"
#define INIT_PREFIX  "init:"

typedef enum {
	LIST_TASKS,
	LIST_THREADS,
	LIST_IPCCS,
	LIST_CPUS,
	PRINT_LOAD,
	PRINT_UPTIME,
	PRINT_ARCH
} output_toggle_t;

static void list_tasks(void)
{
	size_t count;
	stats_task_t *stats_tasks = stats_get_tasks(&count);

	if (stats_tasks == NULL) {
		fprintf(stderr, "%s: Unable to get tasks\n", NAME);
		return;
	}

	printf("[taskid] [thrds] [resident] [virtual] [ucycles]"
	    " [kcycles] [name\n");

	for (size_t i = 0; i < count; i++) {
		uint64_t resmem;
		uint64_t virtmem;
		uint64_t ucycles;
		uint64_t kcycles;
		const char *resmem_suffix;
		const char *virtmem_suffix;
		char usuffix;
		char ksuffix;

		bin_order_suffix(stats_tasks[i].resmem, &resmem, &resmem_suffix, true);
		bin_order_suffix(stats_tasks[i].virtmem, &virtmem, &virtmem_suffix, true);
		order_suffix(stats_tasks[i].ucycles, &ucycles, &usuffix);
		order_suffix(stats_tasks[i].kcycles, &kcycles, &ksuffix);

		printf("%-8" PRIu64 " %7zu %7" PRIu64 "%s %6" PRIu64 "%s"
		    " %8" PRIu64 "%c %8" PRIu64 "%c %s\n",
		    stats_tasks[i].task_id, stats_tasks[i].threads,
		    resmem, resmem_suffix, virtmem, virtmem_suffix,
		    ucycles, usuffix, kcycles, ksuffix, stats_tasks[i].name);
	}

	free(stats_tasks);
}

static void list_threads(task_id_t task_id, bool all)
{
	size_t count;
	stats_thread_t *stats_threads = stats_get_threads(&count);

	if (stats_threads == NULL) {
		fprintf(stderr, "%s: Unable to get threads\n", NAME);
		return;
	}

	printf("[taskid] [threadid] [state ] [prio] [cpu ] [ucycles] [kcycles]\n");

	for (size_t i = 0; i < count; i++) {
		if ((all) || (stats_threads[i].task_id == task_id)) {
			uint64_t ucycles, kcycles;
			char usuffix, ksuffix;

			order_suffix(stats_threads[i].ucycles, &ucycles, &usuffix);
			order_suffix(stats_threads[i].kcycles, &kcycles, &ksuffix);

			printf("%-8" PRIu64 " %-10" PRIu64 " %-8s %6d ",
			    stats_threads[i].task_id, stats_threads[i].thread_id,
			    thread_get_state(stats_threads[i].state),
			    stats_threads[i].priority);

			if (stats_threads[i].on_cpu)
				printf("%6u ", stats_threads[i].cpu);
			else
				printf("(none) ");

			printf("%8" PRIu64 "%c %8" PRIu64 "%c\n",
			    ucycles, usuffix, kcycles, ksuffix);
		}
	}

	free(stats_threads);
}

static void list_ipccs(task_id_t task_id, bool all)
{
	size_t count;
	stats_ipcc_t *stats_ipccs = stats_get_ipccs(&count);

	if (stats_ipccs == NULL) {
		fprintf(stderr, "%s: Unable to get IPC connections\n", NAME);
		return;
	}

	printf("[caller] [callee]\n");

	for (size_t i = 0; i < count; i++) {
		if ((all) || (stats_ipccs[i].caller == task_id)) {
			printf("%-8" PRIu64 " %-8" PRIu64 "\n",
			    stats_ipccs[i].caller, stats_ipccs[i].callee);
		}
	}

	free(stats_ipccs);
}

static void list_cpus(void)
{
	size_t count;
	stats_cpu_t *cpus = stats_get_cpus(&count);

	if (cpus == NULL) {
		fprintf(stderr, "%s: Unable to get CPU statistics\n", NAME);
		return;
	}

	printf("[id] [MHz     ] [busy cycles] [idle cycles]\n");

	for (size_t i = 0; i < count; i++) {
		printf("%-4u ", cpus[i].id);
		if (cpus[i].active) {
			uint64_t bcycles, icycles;
			char bsuffix, isuffix;

			order_suffix(cpus[i].busy_cycles, &bcycles, &bsuffix);
			order_suffix(cpus[i].idle_cycles, &icycles, &isuffix);

			printf("%10" PRIu16 " %12" PRIu64 "%c %12" PRIu64 "%c\n",
			    cpus[i].frequency_mhz, bcycles, bsuffix,
			    icycles, isuffix);
		} else
			printf("inactive\n");
	}

	free(cpus);
}

static void print_load(void)
{
	size_t count;
	load_t *load = stats_get_load(&count);

	if (load == NULL) {
		fprintf(stderr, "%s: Unable to get load\n", NAME);
		return;
	}

	printf("%s: Load average: ", NAME);

	for (size_t i = 0; i < count; i++) {
		if (i > 0)
			printf(" ");

		stats_print_load_fragment(load[i], 2);
	}

	printf("\n");

	free(load);
}

static void print_uptime(void)
{
	struct timespec uptime;
	getuptime(&uptime);

	printf("%s: Up %lld days, %lld hours, %lld minutes, %lld seconds\n",
	    NAME, uptime.tv_sec / DAY, (uptime.tv_sec % DAY) / HOUR,
	    (uptime.tv_sec % HOUR) / MINUTE, uptime.tv_sec % MINUTE);
}

static char *escape_dot(const char *str)
{
	size_t size = 0;
	for (size_t i = 0; str[i] != 0; i++) {
		if (str[i] == '"')
			size++;

		size++;
	}

	char *escaped_str = calloc(size + 1, sizeof(char));
	if (escaped_str == NULL)
		return NULL;

	size_t pos = 0;
	for (size_t i = 0; str[i] != 0; i++) {
		if (str[i] == '"') {
			escaped_str[pos] = '\\';
			pos++;
		}

		escaped_str[pos] = str[i];
		pos++;
	}

	escaped_str[pos] = 0;

	return escaped_str;
}

static void print_arch(void)
{
	size_t count_tasks;
	stats_task_t *stats_tasks = stats_get_tasks(&count_tasks);

	if (stats_tasks == NULL) {
		fprintf(stderr, "%s: Unable to get tasks\n", NAME);
		return;
	}

	size_t count_ipccs;
	stats_ipcc_t *stats_ipccs = stats_get_ipccs(&count_ipccs);

	if (stats_ipccs == NULL) {
		fprintf(stderr, "%s: Unable to get IPC connections\n", NAME);
		return;
	}

	/* Global dot language attributes */
	printf("digraph HelenOS {\n");
	printf("\tlayout=sfdp\n");
	printf("\t// layout=neato\n");
	printf("\tsplines=true\n");
	printf("\t// splines=ortho\n");
	printf("\tconcentrate=true\n");
	printf("\tcenter=true\n");
	printf("\toverlap=false\n");
	printf("\toutputorder=edgesfirst\n");
	printf("\tfontsize=12\n");
	printf("\tnode [shape=component style=filled color=red "
	    "fillcolor=yellow]\n\t\n");

	bool kernel_found = false;
	task_id_t kernel_id = 0;

	/* Tasks as vertices (components) */
	for (size_t i = 0; i < count_tasks; i++) {
		/* Kernel task */
		bool kernel = (str_cmp(stats_tasks[i].name, KERNEL_NAME) == 0);

		/* Init task */
		bool init = str_test_prefix(stats_tasks[i].name, INIT_PREFIX);

		char *escaped_name = NULL;

		if (init)
			escaped_name = escape_dot(str_suffix(stats_tasks[i].name,
			    str_length(INIT_PREFIX)));
		else
			escaped_name = escape_dot(stats_tasks[i].name);

		if (escaped_name == NULL)
			continue;

		if (kernel) {
			if (kernel_found) {
				fprintf(stderr, "%s: Duplicate kernel tasks\n", NAME);
			} else {
				kernel_found = true;
				kernel_id = stats_tasks[i].task_id;
			}

			printf("\ttask%" PRIu64 " [label=\"%s\" shape=invtrapezium "
			    "fillcolor=gold]\n", stats_tasks[i].task_id, escaped_name);
		} else if (init)
			printf("\ttask%" PRIu64 " [label=\"%s\" fillcolor=orange]\n",
			    stats_tasks[i].task_id, escaped_name);
		else
			printf("\ttask%" PRIu64 " [label=\"%s\"]\n", stats_tasks[i].task_id,
			    escaped_name);

		free(escaped_name);
	}

	printf("\t\n");

	if (kernel_found) {
		/*
		 * Add an invisible edge from all user
		 * space tasks to the kernel to increase
		 * the kernel ranking.
		 */

		for (size_t i = 0; i < count_tasks; i++) {
			/* Skip the kernel itself */
			if (stats_tasks[i].task_id == kernel_id)
				continue;

			printf("\ttask%" PRIu64 " -> task%" PRIu64 " [style=\"invis\"]\n",
			    stats_tasks[i].task_id, kernel_id);
		}
	}

	printf("\t\n");

	/* IPC connections as edges */
	for (size_t i = 0; i < count_ipccs; i++) {
		printf("\ttask%" PRIu64 " -> task%" PRIu64 "\n",
		    stats_ipccs[i].caller, stats_ipccs[i].callee);
	}

	printf("}\n");

	free(stats_tasks);
	free(stats_ipccs);
}

static void usage(const char *name)
{
	printf(
	    "Usage: %s [-t task_id] [-i task_id] [-at] [-ai] [-c] [-l] [-u] [-d]\n"
	    "\n"
	    "Options:\n"
	    "\t-t task_id | --task=task_id\n"
	    "\t\tList threads of the given task\n"
	    "\n"
	    "\t-i task_id | --ipcc=task_id\n"
	    "\t\tList IPC connections of the given task\n"
	    "\n"
	    "\t-at | --all-threads\n"
	    "\t\tList all threads\n"
	    "\n"
	    "\t-ai | --all-ipccs\n"
	    "\t\tList all IPC connections\n"
	    "\n"
	    "\t-c | --cpus\n"
	    "\t\tList CPUs\n"
	    "\n"
	    "\t-l | --load\n"
	    "\t\tPrint system load\n"
	    "\n"
	    "\t-u | --uptime\n"
	    "\t\tPrint system uptime\n"
	    "\n"
	    "\t-d | --design\n"
	    "\t\tPrint the current system architecture graph\n"
	    "\n"
	    "\t-h | --help\n"
	    "\t\tPrint this usage information\n"
	    "\n"
	    "Without any options all tasks are listed\n",
	    name);
}

int main(int argc, char *argv[])
{
	output_toggle_t output_toggle = LIST_TASKS;
	bool toggle_all = false;
	task_id_t task_id = 0;

	for (int i = 1; i < argc; i++) {
		int off;

		/* Usage */
		if ((off = arg_parse_short_long(argv[i], "-h", "--help")) != -1) {
			usage(argv[0]);
			return 0;
		}

		/* All IPC connections */
		if ((off = arg_parse_short_long(argv[i], "-ai", "--all-ipccs")) != -1) {
			output_toggle = LIST_IPCCS;
			toggle_all = true;
			continue;
		}

		/* All threads */
		if ((off = arg_parse_short_long(argv[i], "-at", "--all-threads")) != -1) {
			output_toggle = LIST_THREADS;
			toggle_all = true;
			continue;
		}

		/* IPC connections */
		if ((off = arg_parse_short_long(argv[i], "-i", "--ipcc=")) != -1) {
			// TODO: Support for 64b range
			int tmp;
			errno_t ret = arg_parse_int(argc, argv, &i, &tmp, off);
			if (ret != EOK) {
				printf("%s: Malformed task id '%s'\n", NAME, argv[i]);
				return -1;
			}

			task_id = tmp;

			output_toggle = LIST_IPCCS;
			continue;
		}

		/* Tasks */
		if ((off = arg_parse_short_long(argv[i], "-t", "--task=")) != -1) {
			// TODO: Support for 64b range
			int tmp;
			errno_t ret = arg_parse_int(argc, argv, &i, &tmp, off);
			if (ret != EOK) {
				printf("%s: Malformed task id '%s'\n", NAME, argv[i]);
				return -1;
			}

			task_id = tmp;

			output_toggle = LIST_THREADS;
			continue;
		}

		/* CPUs */
		if ((off = arg_parse_short_long(argv[i], "-c", "--cpus")) != -1) {
			output_toggle = LIST_CPUS;
			continue;
		}

		/* Load */
		if ((off = arg_parse_short_long(argv[i], "-l", "--load")) != -1) {
			output_toggle = PRINT_LOAD;
			continue;
		}

		/* Uptime */
		if ((off = arg_parse_short_long(argv[i], "-u", "--uptime")) != -1) {
			output_toggle = PRINT_UPTIME;
			continue;
		}

		/* Architecture */
		if ((off = arg_parse_short_long(argv[i], "-d", "--design")) != -1) {
			output_toggle = PRINT_ARCH;
			continue;
		}
	}

	switch (output_toggle) {
	case LIST_TASKS:
		list_tasks();
		break;
	case LIST_THREADS:
		list_threads(task_id, toggle_all);
		break;
	case LIST_IPCCS:
		list_ipccs(task_id, toggle_all);
		break;
	case LIST_CPUS:
		list_cpus();
		break;
	case PRINT_LOAD:
		print_load();
		break;
	case PRINT_UPTIME:
		print_uptime();
		break;
	case PRINT_ARCH:
		print_arch();
		break;
	}

	return 0;
}

/** @}
 */
