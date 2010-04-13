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
 * @brief Top utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uptime.h>
#include <task.h>
#include <thread.h>
#include <sys/time.h>
#include <load.h>
#include <ps.h>
#include <arch/barrier.h>
#include "screen.h"
#include "input.h"
#include "top.h"
#include "ps.h"

#define UPDATE_INTERVAL 1

#define DAY 86400
#define HOUR 3600
#define MINUTE 60

int number = 0;
int number2 = 0;
static void read_data(data_t *target)
{
	/* Read current time */
	struct timeval time;
	if (gettimeofday(&time, NULL) != 0) {
		printf("Cannot get time of day!\n");
		exit(1);
	}
	target->hours = (time.tv_sec % DAY) / HOUR;
	target->minutes = (time.tv_sec % HOUR) / MINUTE;
	target->seconds = time.tv_sec % MINUTE;

	/* Read uptime */
	uint64_t uptime;
	get_uptime(&uptime);
	target->uptime_d = uptime / DAY;
	target->uptime_h = (uptime % DAY) / HOUR;
	target->uptime_m = (uptime % HOUR) / MINUTE;
	target->uptime_s = uptime % MINUTE;

	/* Read load */
	get_load(target->load);

	/* Read task ids */
	target->task_count = get_tasks(&target->taskinfos);

	/* Read all threads */
	target->thread_count = get_threads(&target->thread_infos);

	/* Read cpu infos */
	target->cpu_count = get_cpu_infos(&target->cpus);

	/* Read mem info */
	get_mem_info(&target->mem_info);
}

/** Computes percentage differencies from old_data to new_data
 *
 * @param old_data	Pointer to old data strucutre.
 * @param new_data	Pointer to actual data where percetages are stored.
 *
 */
static void compute_percentages(data_t *old_data, data_t *new_data)
{
	/* Foreach cpu, compute total ticks and divide it between user and
	 * system */
	unsigned int i;
	new_data->cpu_perc = malloc(new_data->cpu_count * sizeof(cpu_perc_t));
	for (i = 0; i < new_data->cpu_count; ++i) {
		uint64_t idle = new_data->cpus[i].idle_ticks - old_data->cpus[i].idle_ticks;
		uint64_t busy = new_data->cpus[i].busy_ticks - old_data->cpus[i].busy_ticks;
		uint64_t sum = idle + busy;
		FRACTION_TO_FLOAT(new_data->cpu_perc[i].idle, idle * 100, sum);
		FRACTION_TO_FLOAT(new_data->cpu_perc[i].busy, busy * 100, sum);
	}

	/* For all tasks compute sum and differencies of all cycles */
	uint64_t mem_total = 1; /*< Must NOT be null! */
	uint64_t ucycles_total = 1; /*< Must NOT be null! */
	uint64_t kcycles_total = 1; /*< Must NOT be null! */
	uint64_t *ucycles_diff = malloc(new_data->task_count * sizeof(uint64_t));
	uint64_t *kcycles_diff = malloc(new_data->task_count * sizeof(uint64_t));
	unsigned int j = 0;
	for (i = 0; i < new_data->task_count; ++i) {
		/* Jump over all death tasks */
		while (old_data->taskinfos[j].taskid < new_data->taskinfos[i].taskid)
			++j;
		if (old_data->taskinfos[j].taskid > new_data->taskinfos[i].taskid) {
			/* This is newly borned task, ignore it */
			ucycles_diff[i] = 0;
			kcycles_diff[i] = 0;
			continue;
		}
		/* Now we now we have task with same id */
		ucycles_diff[i] = new_data->taskinfos[i].ucycles - old_data->taskinfos[j].ucycles;
		kcycles_diff[i] = new_data->taskinfos[i].kcycles - old_data->taskinfos[j].kcycles;

		mem_total += new_data->taskinfos[i].virt_mem;
		ucycles_total += ucycles_diff[i];
		kcycles_total += kcycles_diff[i];
	}

	/* And now compute percental change */
	new_data->task_perc = malloc(new_data->task_count * sizeof(task_perc_t));
	for (i = 0; i < new_data->task_count; ++i) {
		FRACTION_TO_FLOAT(new_data->task_perc[i].mem, new_data->taskinfos[i].virt_mem * 100, mem_total);
		FRACTION_TO_FLOAT(new_data->task_perc[i].ucycles, ucycles_diff[i] * 100, ucycles_total);
		FRACTION_TO_FLOAT(new_data->task_perc[i].kcycles, kcycles_diff[i] * 100, kcycles_total);
	}

	/* Wait until coprocessor finishes its work */
	write_barrier();

	/* And free temporary structures */
	free(ucycles_diff);
	free(kcycles_diff);
}

static void free_data(data_t *target)
{
	free(target->taskinfos);
	free(target->thread_infos);
	free(target->cpus);
	free(target->cpu_perc);
	free(target->task_perc);
}

static inline void swap(data_t **first, data_t **second)
{
	data_t *temp;
	temp = *first;
	*first = *second;
	*second = temp;
}

static data_t data[2];

int main(int argc, char *argv[])
{
	data_t *data1 = &data[0];
	data_t *data2 = &data[1];
	screen_init();

	/* Read initial stats */
	printf("Reading initial data...\n");
	read_data(data1);
	/* Compute some rubbish to have initialised values */
	compute_percentages(data1, data1);

	/* And paint screen until death... */
	while (true) {
		char c = tgetchar(UPDATE_INTERVAL);
		if (c < 0) {
			read_data(data2);
			compute_percentages(data1, data2);
			free_data(data1);
			print_data(data2);
			swap(&data1, &data2);
			continue;
		}
		switch (c) {
			case 'q':
				clear_screen();
				return 0;
			default:
				PRINT_WARNING("Unknown command: %c", c);
				break;
		}

	}

	free_data(data1);
	free_data(data2);
	return 0;
}

/** @}
 */
