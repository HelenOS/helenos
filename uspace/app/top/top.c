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

/** @addtogroup top
 * @brief Top utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <task.h>
#include <thread.h>
#include <sys/time.h>
#include <errno.h>
#include <sort.h>
#include "screen.h"
#include "top.h"

#define NAME  "top"

#define UPDATE_INTERVAL  1

#define DAY     86400
#define HOUR    3600
#define MINUTE  60

op_mode_t op_mode = OP_TASKS;
sort_mode_t sort_mode = SORT_TASK_CYCLES;
bool excs_all = false;

static const char *read_data(data_t *target)
{
	/* Initialize data */
	target->load = NULL;
	target->cpus = NULL;
	target->cpus_perc = NULL;
	target->tasks = NULL;
	target->tasks_perc = NULL;
	target->tasks_map = NULL;
	target->threads = NULL;
	target->exceptions = NULL;
	target->exceptions_perc = NULL;
	target->physmem = NULL;
	target->ucycles_diff = NULL;
	target->kcycles_diff = NULL;
	target->ecycles_diff = NULL;
	target->ecount_diff = NULL;
	
	/* Get current time */
	struct timeval time;
	if (gettimeofday(&time, NULL) != EOK)
		return "Cannot get time of day";
	
	target->hours = (time.tv_sec % DAY) / HOUR;
	target->minutes = (time.tv_sec % HOUR) / MINUTE;
	target->seconds = time.tv_sec % MINUTE;
	
	/* Get uptime */
	sysarg_t uptime = stats_get_uptime();
	target->udays = uptime / DAY;
	target->uhours = (uptime % DAY) / HOUR;
	target->uminutes = (uptime % HOUR) / MINUTE;
	target->useconds = uptime % MINUTE;
	
	/* Get load */
	target->load = stats_get_load(&(target->load_count));
	if (target->load == NULL)
		return "Cannot get system load";
	
	/* Get CPUs */
	target->cpus = stats_get_cpus(&(target->cpus_count));
	if (target->cpus == NULL)
		return "Cannot get CPUs";
	
	target->cpus_perc =
	    (perc_cpu_t *) calloc(target->cpus_count, sizeof(perc_cpu_t));
	if (target->cpus_perc == NULL)
		return "Not enough memory for CPU utilization";
	
	/* Get tasks */
	target->tasks = stats_get_tasks(&(target->tasks_count));
	if (target->tasks == NULL)
		return "Cannot get tasks";
	
	target->tasks_perc =
	    (perc_task_t *) calloc(target->tasks_count, sizeof(perc_task_t));
	if (target->tasks_perc == NULL)
		return "Not enough memory for task utilization";
	
	target->tasks_map =
	    (size_t *) calloc(target->tasks_count, sizeof(size_t));
	if (target->tasks_map == NULL)
		return "Not enough memory for task map";
	
	/* Get threads */
	target->threads = stats_get_threads(&(target->threads_count));
	if (target->threads == NULL)
		return "Cannot get threads";
	
	/* Get Exceptions */
	target->exceptions = stats_get_exceptions(&(target->exceptions_count));
	if (target->exceptions == NULL)
		return "Cannot get exceptions";
	
	target->exceptions_perc =
	    (perc_exc_t *) calloc(target->exceptions_count, sizeof(perc_exc_t));
	if (target->exceptions_perc == NULL)
		return "Not enough memory for exception utilization";
	
	/* Get physical memory */
	target->physmem = stats_get_physmem();
	if (target->physmem == NULL)
		return "Cannot get physical memory";
	
	target->ucycles_diff = calloc(target->tasks_count,
	    sizeof(uint64_t));
	if (target->ucycles_diff == NULL)
		return "Not enough memory for user utilization";
	
	/* Allocate memory for computed values */
	target->kcycles_diff = calloc(target->tasks_count,
	    sizeof(uint64_t));
	if (target->kcycles_diff == NULL)
		return "Not enough memory for kernel utilization";
	
	target->ecycles_diff = calloc(target->exceptions_count,
	    sizeof(uint64_t));
	if (target->ecycles_diff == NULL)
		return "Not enough memory for exception cycles utilization";
	
	target->ecount_diff = calloc(target->exceptions_count,
	    sizeof(uint64_t));
	if (target->ecount_diff == NULL)
		return "Not enough memory for exception count utilization";
	
	return NULL;
}

/** Computes percentage differencies from old_data to new_data
 *
 * @param old_data Pointer to old data strucutre.
 * @param new_data Pointer to actual data where percetages are stored.
 *
 */
static void compute_percentages(data_t *old_data, data_t *new_data)
{
	/* For each CPU: Compute total cycles and divide it between
	   user and kernel */
	
	size_t i;
	for (i = 0; i < new_data->cpus_count; i++) {
		uint64_t idle =
		    new_data->cpus[i].idle_cycles - old_data->cpus[i].idle_cycles;
		uint64_t busy =
		    new_data->cpus[i].busy_cycles - old_data->cpus[i].busy_cycles;
		uint64_t sum = idle + busy;
		
		FRACTION_TO_FLOAT(new_data->cpus_perc[i].idle, idle * 100, sum);
		FRACTION_TO_FLOAT(new_data->cpus_perc[i].busy, busy * 100, sum);
	}
	
	/* For all tasks compute sum and differencies of all cycles */
	
	uint64_t virtmem_total = 0;
	uint64_t resmem_total = 0;
	uint64_t ucycles_total = 0;
	uint64_t kcycles_total = 0;
	
	for (i = 0; i < new_data->tasks_count; i++) {
		/* Match task with the previous instance */
		
		bool found = false;
		size_t j;
		for (j = 0; j < old_data->tasks_count; j++) {
			if (new_data->tasks[i].task_id == old_data->tasks[j].task_id) {
				found = true;
				break;
			}
		}
		
		if (!found) {
			/* This is newly borned task, ignore it */
			new_data->ucycles_diff[i] = 0;
			new_data->kcycles_diff[i] = 0;
			continue;
		}
		
		new_data->ucycles_diff[i] =
		    new_data->tasks[i].ucycles - old_data->tasks[j].ucycles;
		new_data->kcycles_diff[i] =
		    new_data->tasks[i].kcycles - old_data->tasks[j].kcycles;
		
		virtmem_total += new_data->tasks[i].virtmem;
		resmem_total += new_data->tasks[i].resmem;
		ucycles_total += new_data->ucycles_diff[i];
		kcycles_total += new_data->kcycles_diff[i];
	}
	
	/* For each task compute percential change */
	
	for (i = 0; i < new_data->tasks_count; i++) {
		FRACTION_TO_FLOAT(new_data->tasks_perc[i].virtmem,
		    new_data->tasks[i].virtmem * 100, virtmem_total);
		FRACTION_TO_FLOAT(new_data->tasks_perc[i].resmem,
		    new_data->tasks[i].resmem * 100, resmem_total);
		FRACTION_TO_FLOAT(new_data->tasks_perc[i].ucycles,
		    new_data->ucycles_diff[i] * 100, ucycles_total);
		FRACTION_TO_FLOAT(new_data->tasks_perc[i].kcycles,
		    new_data->kcycles_diff[i] * 100, kcycles_total);
	}
	
	/* For all exceptions compute sum and differencies of cycles */
	
	uint64_t ecycles_total = 0;
	uint64_t ecount_total = 0;
	
	for (i = 0; i < new_data->exceptions_count; i++) {
		/*
		 * March exception with the previous instance.
		 * This is quite paranoid since exceptions do not
		 * usually disappear, but it does not hurt.
		 */
		
		bool found = false;
		size_t j;
		for (j = 0; j < old_data->exceptions_count; j++) {
			if (new_data->exceptions[i].id == old_data->exceptions[j].id) {
				found = true;
				break;
			}
		}
		
		if (!found) {
			/* This is a new exception, ignore it */
			new_data->ecycles_diff[i] = 0;
			new_data->ecount_diff[i] = 0;
			continue;
		}
		
		new_data->ecycles_diff[i] =
		    new_data->exceptions[i].cycles - old_data->exceptions[j].cycles;
		new_data->ecount_diff[i] =
		    new_data->exceptions[i].count - old_data->exceptions[i].count;
		
		ecycles_total += new_data->ecycles_diff[i];
		ecount_total += new_data->ecount_diff[i];
	}
	
	/* For each exception compute percential change */
	
	for (i = 0; i < new_data->exceptions_count; i++) {
		FRACTION_TO_FLOAT(new_data->exceptions_perc[i].cycles,
		    new_data->ecycles_diff[i] * 100, ecycles_total);
		FRACTION_TO_FLOAT(new_data->exceptions_perc[i].count,
		    new_data->ecount_diff[i] * 100, ecount_total);
	}
}

static int cmp_data(void *a, void *b, void *arg)
{
	size_t ia = *((size_t *) a);
	size_t ib = *((size_t *) b);
	data_t *data = (data_t *) arg;
	
	uint64_t acycles = data->ucycles_diff[ia] + data->kcycles_diff[ia];
	uint64_t bcycles = data->ucycles_diff[ib] + data->kcycles_diff[ib];
	
	if (acycles > bcycles)
		return -1;
	
	if (acycles < bcycles)
		return 1;
	
	return 0;
}

static void sort_data(data_t *data)
{
	size_t i;
	
	for (i = 0; i < data->tasks_count; i++)
		data->tasks_map[i] = i;
	
	qsort((void *) data->tasks_map, data->tasks_count,
	    sizeof(size_t), cmp_data, (void *) data);
}

static void free_data(data_t *target)
{
	if (target->load != NULL)
		free(target->load);
	
	if (target->cpus != NULL)
		free(target->cpus);
	
	if (target->cpus_perc != NULL)
		free(target->cpus_perc);
	
	if (target->tasks != NULL)
		free(target->tasks);
	
	if (target->tasks_perc != NULL)
		free(target->tasks_perc);
	
	if (target->threads != NULL)
		free(target->threads);
	
	if (target->exceptions != NULL)
		free(target->exceptions);
	
	if (target->exceptions_perc != NULL)
		free(target->exceptions_perc);
	
	if (target->physmem != NULL)
		free(target->physmem);
	
	if (target->ucycles_diff != NULL)
		free(target->ucycles_diff);
	
	if (target->kcycles_diff != NULL)
		free(target->kcycles_diff);
	
	if (target->ecycles_diff != NULL)
		free(target->ecycles_diff);
	
	if (target->ecount_diff != NULL)
		free(target->ecount_diff);
}

int main(int argc, char *argv[])
{
	data_t data;
	data_t data_prev;
	const char *ret = NULL;
	
	screen_init();
	printf("Reading initial data...\n");
	
	if ((ret = read_data(&data_prev)) != NULL)
		goto out;
	
	/* Compute some rubbish to have initialised values */
	compute_percentages(&data_prev, &data_prev);
	
	/* And paint screen until death */
	while (true) {
		int c = tgetchar(UPDATE_INTERVAL);
		if (c < 0) {
			if ((ret = read_data(&data)) != NULL) {
				free_data(&data);
				goto out;
			}
			
			compute_percentages(&data_prev, &data);
			sort_data(&data);
			print_data(&data);
			free_data(&data_prev);
			data_prev = data;
			
			continue;
		}
		
		switch (c) {
			case 't':
				print_warning("Showing task statistics");
				op_mode = OP_TASKS;
				break;
			case 'i':
				print_warning("Showing IPC statistics");
				op_mode = OP_IPC;
				break;
			case 'e':
				print_warning("Showing exception statistics");
				op_mode = OP_EXCS;
				break;
			case 'h':
				print_warning("Showing help");
				op_mode = OP_HELP;
				break;
			case 'q':
				goto out;
			case 'a':
				if (op_mode == OP_EXCS) {
					excs_all = !excs_all;
					if (excs_all)
						print_warning("Showing all exceptions");
					else
						print_warning("Showing only hot exceptions");
					break;
				}
			default:
				print_warning("Unknown command \"%c\", use \"h\" for help", c);
				break;
		}
	}
	
out:
	screen_done();
	free_data(&data_prev);
	
	if (ret != NULL) {
		fprintf(stderr, "%s: %s\n", NAME, ret);
		return 1;
	}
	
	return 0;
}

/** @}
 */
