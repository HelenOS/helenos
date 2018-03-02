/*
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
