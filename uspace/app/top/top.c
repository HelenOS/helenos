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
#include <task.h>
#include <thread.h>
#include <sys/time.h>
#include <errno.h>
#include <gsort.h>
#include <str.h>
#include "screen.h"
#include "top.h"

#define NAME  "top"

#define UPDATE_INTERVAL  1

#define DAY     86400
#define HOUR    3600
#define MINUTE  60

typedef enum {
	OP_TASKS,
	OP_IPC,
	OP_EXCS,
} op_mode_t;

static const column_t task_columns[] = {
	{"taskid",   't',  8},
	{"thrds",    'h',  7},
	{"resident", 'r', 10},
	{"%resi",    'R',  7},
	{"virtual",  'v',  9},
	{"%virt",    'V',  7},
	{"%user",    'U',  7},
	{"%kern",    'K',  7},
	{"name",     'd',  0},
};

enum {
	TASK_COL_ID = 0,
	TASK_COL_NUM_THREADS,
	TASK_COL_RESIDENT,
	TASK_COL_PERCENT_RESIDENT,
	TASK_COL_VIRTUAL,
	TASK_COL_PERCENT_VIRTUAL,
	TASK_COL_PERCENT_USER,
	TASK_COL_PERCENT_KERNEL,
	TASK_COL_NAME,
	TASK_NUM_COLUMNS,
};

static const column_t ipc_columns[] = {
	{"taskid",  't', 8},
	{"cls snt", 'c', 9},
	{"cls rcv", 'C', 9},
	{"ans snt", 'a', 9},
	{"ans rcv", 'A', 9},
	{"forward", 'f', 9},
	{"name",    'd', 0},
};

enum {
	IPC_COL_TASKID = 0,
	IPC_COL_CLS_SNT,
	IPC_COL_CLS_RCV,
	IPC_COL_ANS_SNT,
	IPC_COL_ANS_RCV,
	IPC_COL_FORWARD,
	IPC_COL_NAME,
	IPC_NUM_COLUMNS,
};

static const column_t exception_columns[] = {
	{"exc",         'e',  8},
	{"count",       'n', 10},
	{"%count",      'N',  8},
	{"cycles",      'c', 10},
	{"%cycles",     'C',  9},
	{"description", 'd',  0},
};

enum {
	EXCEPTION_COL_ID = 0,
	EXCEPTION_COL_COUNT,
	EXCEPTION_COL_PERCENT_COUNT,
	EXCEPTION_COL_CYCLES,
	EXCEPTION_COL_PERCENT_CYCLES,
	EXCEPTION_COL_DESCRIPTION,
	EXCEPTION_NUM_COLUMNS,
};

screen_mode_t screen_mode = SCREEN_TABLE;
static op_mode_t op_mode = OP_TASKS;
static size_t sort_column = TASK_COL_PERCENT_USER;
static int sort_reverse = -1;
static bool excs_all = false;

static const char *read_data(data_t *target)
{
	/* Initialize data */
	target->load = NULL;
	target->cpus = NULL;
	target->cpus_perc = NULL;
	target->tasks = NULL;
	target->tasks_perc = NULL;
	target->threads = NULL;
	target->exceptions = NULL;
	target->exceptions_perc = NULL;
	target->physmem = NULL;
	target->ucycles_diff = NULL;
	target->kcycles_diff = NULL;
	target->ecycles_diff = NULL;
	target->ecount_diff = NULL;
	target->table.name = NULL;
	target->table.num_columns = 0;
	target->table.columns = NULL;
	target->table.num_fields = 0;
	target->table.fields = NULL;
	
	/* Get current time */
	struct timeval time;
	gettimeofday(&time, NULL);
	
	target->hours = (time.tv_sec % DAY) / HOUR;
	target->minutes = (time.tv_sec % HOUR) / MINUTE;
	target->seconds = time.tv_sec % MINUTE;
	
	/* Get uptime */
	struct timeval uptime;
	getuptime(&uptime);
	
	target->udays = uptime.tv_sec / DAY;
	target->uhours = (uptime.tv_sec % DAY) / HOUR;
	target->uminutes = (uptime.tv_sec % HOUR) / MINUTE;
	target->useconds = uptime.tv_sec % MINUTE;
	
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
	field_t *fa = (field_t *)a + sort_column;
	field_t *fb = (field_t *)b + sort_column;
	
	if (fa->type > fb->type)
		return 1 * sort_reverse;

	if (fa->type < fb->type)
		return -1 * sort_reverse;

	switch (fa->type) {
	case FIELD_EMPTY:
		return 0;
	case FIELD_UINT_SUFFIX_BIN: /* fallthrough */
	case FIELD_UINT_SUFFIX_DEC: /* fallthrough */
	case FIELD_UINT:
		if (fa->uint > fb->uint)
			return 1 * sort_reverse;
		if (fa->uint < fb->uint)
			return -1 * sort_reverse;
		return 0;
	case FIELD_PERCENT:
		if (fa->fixed.upper * fb->fixed.lower
		    > fb->fixed.upper * fa->fixed.lower)
			return 1 * sort_reverse;
		if (fa->fixed.upper * fb->fixed.lower
		    < fb->fixed.upper * fa->fixed.lower)
			return -1 * sort_reverse;
		return 0;
	case FIELD_STRING:
		return str_cmp(fa->string, fb->string) * sort_reverse;
	}

	return 0;
}

static void sort_table(table_t *table)
{
	if (sort_column >= table->num_columns)
		sort_column = 0;
	/* stable sort is probably best, so we use gsort */
	gsort((void *) table->fields, table->num_fields / table->num_columns,
	    sizeof(field_t) * table->num_columns, cmp_data, NULL);
}

static const char *fill_task_table(data_t *data)
{
	data->table.name = "Tasks";
	data->table.num_columns = TASK_NUM_COLUMNS;
	data->table.columns = task_columns;
	data->table.num_fields = data->tasks_count * TASK_NUM_COLUMNS;
	data->table.fields = calloc(data->table.num_fields,
	    sizeof(field_t));
	if (data->table.fields == NULL)
		return "Not enough memory for table fields";

	field_t *field = data->table.fields;
	for (size_t i = 0; i < data->tasks_count; i++) {
		stats_task_t *task = &data->tasks[i];
		perc_task_t *perc = &data->tasks_perc[i];
		field[TASK_COL_ID].type = FIELD_UINT;
		field[TASK_COL_ID].uint = task->task_id;
		field[TASK_COL_NUM_THREADS].type = FIELD_UINT;
		field[TASK_COL_NUM_THREADS].uint = task->threads;
		field[TASK_COL_RESIDENT].type = FIELD_UINT_SUFFIX_BIN;
		field[TASK_COL_RESIDENT].uint = task->resmem;
		field[TASK_COL_PERCENT_RESIDENT].type = FIELD_PERCENT;
		field[TASK_COL_PERCENT_RESIDENT].fixed = perc->resmem;
		field[TASK_COL_VIRTUAL].type = FIELD_UINT_SUFFIX_BIN;
		field[TASK_COL_VIRTUAL].uint = task->virtmem;
		field[TASK_COL_PERCENT_VIRTUAL].type = FIELD_PERCENT;
		field[TASK_COL_PERCENT_VIRTUAL].fixed = perc->virtmem;
		field[TASK_COL_PERCENT_USER].type = FIELD_PERCENT;
		field[TASK_COL_PERCENT_USER].fixed = perc->ucycles;
		field[TASK_COL_PERCENT_KERNEL].type = FIELD_PERCENT;
		field[TASK_COL_PERCENT_KERNEL].fixed = perc->kcycles;
		field[TASK_COL_NAME].type = FIELD_STRING;
		field[TASK_COL_NAME].string = task->name;
		field += TASK_NUM_COLUMNS;
	}

	return NULL;
}

static const char *fill_ipc_table(data_t *data)
{
	data->table.name = "IPC";
	data->table.num_columns = IPC_NUM_COLUMNS;
	data->table.columns = ipc_columns;
	data->table.num_fields = data->tasks_count * IPC_NUM_COLUMNS;
	data->table.fields = calloc(data->table.num_fields,
	    sizeof(field_t));
	if (data->table.fields == NULL)
		return "Not enough memory for table fields";

	field_t *field = data->table.fields;
	for (size_t i = 0; i < data->tasks_count; i++) {
		field[IPC_COL_TASKID].type = FIELD_UINT;
		field[IPC_COL_TASKID].uint = data->tasks[i].task_id;
		field[IPC_COL_CLS_SNT].type = FIELD_UINT_SUFFIX_DEC;
		field[IPC_COL_CLS_SNT].uint = data->tasks[i].ipc_info.call_sent;
		field[IPC_COL_CLS_RCV].type = FIELD_UINT_SUFFIX_DEC;
		field[IPC_COL_CLS_RCV].uint = data->tasks[i].ipc_info.call_received;
		field[IPC_COL_ANS_SNT].type = FIELD_UINT_SUFFIX_DEC;
		field[IPC_COL_ANS_SNT].uint = data->tasks[i].ipc_info.answer_sent;
		field[IPC_COL_ANS_RCV].type = FIELD_UINT_SUFFIX_DEC;
		field[IPC_COL_ANS_RCV].uint = data->tasks[i].ipc_info.answer_received;
		field[IPC_COL_FORWARD].type = FIELD_UINT_SUFFIX_DEC;
		field[IPC_COL_FORWARD].uint = data->tasks[i].ipc_info.forwarded;
		field[IPC_COL_NAME].type = FIELD_STRING;
		field[IPC_COL_NAME].string = data->tasks[i].name;
		field += IPC_NUM_COLUMNS;
	}

	return NULL;
}

static const char *fill_exception_table(data_t *data)
{
	data->table.name = "Exceptions";
	data->table.num_columns = EXCEPTION_NUM_COLUMNS;
	data->table.columns = exception_columns;
	data->table.num_fields = data->exceptions_count *
	    EXCEPTION_NUM_COLUMNS;
	data->table.fields = calloc(data->table.num_fields, sizeof(field_t));
	if (data->table.fields == NULL)
		return "Not enough memory for table fields";

	field_t *field = data->table.fields;
	for (size_t i = 0; i < data->exceptions_count; i++) {
		if (!excs_all && !data->exceptions[i].hot)
			continue;
		field[EXCEPTION_COL_ID].type = FIELD_UINT;
		field[EXCEPTION_COL_ID].uint = data->exceptions[i].id;
		field[EXCEPTION_COL_COUNT].type = FIELD_UINT_SUFFIX_DEC;
		field[EXCEPTION_COL_COUNT].uint = data->exceptions[i].count;
		field[EXCEPTION_COL_PERCENT_COUNT].type = FIELD_PERCENT;
		field[EXCEPTION_COL_PERCENT_COUNT].fixed = data->exceptions_perc[i].count;
		field[EXCEPTION_COL_CYCLES].type = FIELD_UINT_SUFFIX_DEC;
		field[EXCEPTION_COL_CYCLES].uint = data->exceptions[i].cycles;
		field[EXCEPTION_COL_PERCENT_CYCLES].type = FIELD_PERCENT;
		field[EXCEPTION_COL_PERCENT_CYCLES].fixed = data->exceptions_perc[i].cycles;
		field[EXCEPTION_COL_DESCRIPTION].type = FIELD_STRING;
		field[EXCEPTION_COL_DESCRIPTION].string = data->exceptions[i].desc;
		field += EXCEPTION_NUM_COLUMNS;
	}

	/* in case any cold exceptions were ignored */
	data->table.num_fields = field - data->table.fields;

	return NULL;
}

static const char *fill_table(data_t *data)
{
	if (data->table.fields != NULL) {
		free(data->table.fields);
		data->table.fields = NULL;
	}

	switch (op_mode) {
	case OP_TASKS:
		return fill_task_table(data);
	case OP_IPC:
		return fill_ipc_table(data);
	case OP_EXCS:
		return fill_exception_table(data);
	}
	return NULL;
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

	if (target->table.fields != NULL)
		free(target->table.fields);
}

int main(int argc, char *argv[])
{
	data_t data;
	data_t data_prev;
	const char *ret = NULL;
	
	screen_init();
	printf("Reading initial data...\n");
	
	if ((ret = read_data(&data)) != NULL)
		goto out;
	
	/* Compute some rubbish to have initialised values */
	compute_percentages(&data, &data);
	
	/* And paint screen until death */
	while (true) {
		int c = tgetchar(UPDATE_INTERVAL);

		if (c < 0) { /* timeout */
			data_prev = data;
			if ((ret = read_data(&data)) != NULL) {
				free_data(&data_prev);
				goto out;
			}
			
			compute_percentages(&data_prev, &data);
			free_data(&data_prev);

			c = -1;
		}

		if (screen_mode == SCREEN_HELP && c >= 0) {
			if (c == 'h' || c == '?')
				c = -1;
			/* go back to table and handle the key */
			screen_mode = SCREEN_TABLE;
		}

		if (screen_mode == SCREEN_SORT && c >= 0) {
			for (size_t i = 0; i < data.table.num_columns; i++) {
				if (data.table.columns[i].key == c) {
					sort_column = i;
					screen_mode = SCREEN_TABLE;
				}
			}

			c = -1;
		}

		switch (c) {
		case -1: /* do nothing */
			break;
		case 't':
			op_mode = OP_TASKS;
			break;
		case 'i':
			op_mode = OP_IPC;
			break;
		case 'e':
			op_mode = OP_EXCS;
			break;
		case 's':
			screen_mode = SCREEN_SORT;
			break;
		case 'r':
			sort_reverse = -sort_reverse;
			break;
		case 'h':
		case '?':
			screen_mode = SCREEN_HELP;
			break;
		case 'q':
			goto out;
		case 'a':
			if (op_mode == OP_EXCS) {
				excs_all = !excs_all;
				if (excs_all)
					show_warning("Showing all exceptions");
				else
					show_warning("Showing only hot exceptions");
				break;
			}
			/* Fallthrough */
		default:
			show_warning("Unknown command \"%c\", use \"h\" for help", c);
			continue; /* don't redraw */
		}

		if ((ret = fill_table(&data)) != NULL) {
			goto out;
		}
		sort_table(&data.table);
		print_data(&data);
	}
	
out:
	screen_done();
	free_data(&data);
	
	if (ret != NULL) {
		fprintf(stderr, "%s: %s\n", NAME, ret);
		return 1;
	}
	
	return 0;
}

/** @}
 */
