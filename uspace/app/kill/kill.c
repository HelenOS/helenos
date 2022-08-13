/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kill
 * @{
 */
/**
 * @file Forcefully terminate a task.
 */

#include <errno.h>
#include <stdio.h>
#include <task.h>
#include <str.h>
#include <str_error.h>

#define NAME  "kill"

static void print_syntax(void)
{
	printf("Syntax: " NAME " <task ID>\n");
}

int main(int argc, char *argv[])
{
	char *eptr;
	task_id_t taskid;
	errno_t rc;

	if (argc != 2) {
		print_syntax();
		return 1;
	}

	taskid = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		printf("Invalid task ID argument '%s'.\n", argv[1]);
		return 2;
	}

	rc = task_kill(taskid);
	if (rc != EOK) {
		printf("Failed to kill task ID %" PRIu64 ": %s\n",
		    taskid, str_error(rc));
		return 3;
	}

	return 0;
}

/** @}
 */
