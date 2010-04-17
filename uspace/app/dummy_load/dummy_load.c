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

/** @addtogroup dummy_load
 * @brief Dummy application to make some system load.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <task.h>
#include <malloc.h>
#include <ps.h>
#include <str.h>
#include <stdlib.h>
#include "input.h"

#define TASK_COUNT 50
#define USPACE_CYCLES (1 << 26)
#define SYSTEM_CYCLES (1 << 16)

static void uspace_load()
{
	puts("u");
	uint64_t volatile i;
	for (i = 0; i < USPACE_CYCLES; ++i)
		;
}

static int task_count;

static void system_init()
{
	task_count = TASK_COUNT;
	task_id_t *tasks = malloc(task_count * sizeof(task_id_t));
	int result = get_task_ids(tasks, sizeof(task_id_t) * task_count);

	while (result > task_count) {
		task_count *= 2;
		tasks = realloc(tasks, task_count * sizeof(task_id_t));
		result = get_task_ids(tasks, sizeof(task_id_t) * task_count);
	}

	free(tasks);
}

static void system_load()
{
	puts("s");
	task_id_t *tasks = malloc(task_count * sizeof(task_id_t));
	int result;

	uint64_t i;
	for (i = 0; i < SYSTEM_CYCLES; ++i) {
		result = get_task_ids(tasks, sizeof(task_id_t) * task_count);
	}

	free(tasks);
}

static void usage()
{
	printf("Usage: dummy_load [-u|-s|-r]\n");
}

int main(int argc, char *argv[])
{
	--argc; ++argv;
	system_init();

	char mode = 'r';
	if (argc > 1) {
		printf("Bad argument count!\n");
		usage();
		exit(1);
	}

	if (argc > 0) {
		if (str_cmp(*argv, "-r") == 0) {
			printf("Doing random load\n");
			mode = 'r';
		} else if (str_cmp(*argv, "-u") == 0) {
			printf("Doing uspace load\n");
			mode = 'u';
		} else if (str_cmp(*argv, "-s") == 0) {
			printf("Doing system load\n");
			mode = 's';
		} else {
			usage();
			exit(1);
		}
	} else {
		mode = 'r';
		printf("Doing random load\n");
	}

	bool system = false;
	while (true) {
		char c = tgetchar(0);
		switch (c) {
			case 'r':
				mode = 'r';
				break;
			case 'u':
				mode = 'u';
				break;
			case 's':
				mode = 's';
				break;
			case 'q':
				exit(0);
				break;
		}
		switch (mode) {
			case 'r':
				if (system) {
					system_load();
				} else {
					uspace_load();
				}
				system = !system;
				break;
			case 'u':
				uspace_load();
				break;
			case 's':
				system_load();
				break;
		}
		fflush(stdout);
	}

	puts("\n");
	fflush(stdout);
	return 0;
}

/** @}
 */
