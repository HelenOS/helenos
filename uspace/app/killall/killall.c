/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup killall
 * @{
 */
/**
 * @file Forcefully terminate a task specified by name.
 */

#include <errno.h>
#include <stdio.h>
#include <task.h>
#include <stats.h>
#include <str_error.h>
#include <stdlib.h>
#include <str.h>

#define NAME  "killall"

static void print_syntax(void)
{
	printf("Syntax: " NAME " <task name>\n");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		print_syntax();
		return 1;
	}

	size_t count;
	stats_task_t *stats_tasks = stats_get_tasks(&count);

	if (stats_tasks == NULL) {
		fprintf(stderr, "%s: Unable to get tasks\n", NAME);
		return 2;
	}

	size_t i;
	for (i = 0; i < count; i++) {
		if (str_cmp(stats_tasks[i].name, argv[1]) == 0) {
			task_id_t taskid = stats_tasks[i].task_id;
			errno_t rc = task_kill(taskid);
			if (rc != EOK)
				printf("Failed to kill task ID %" PRIu64 ": %s\n",
				    taskid, str_error(rc));
			else
				printf("Killed task ID %" PRIu64 "\n", taskid);
		}
	}

	free(stats_tasks);

	return 0;
}

/** @}
 */
