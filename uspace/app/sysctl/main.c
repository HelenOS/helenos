/*
 * Copyright (c) 2015 Michal Koutny
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

/** @addtogroup sysctl
 * @{
 */
/** @file Control system manager.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <sysman/ctl.h>
#include <fibril.h>

#define NAME "sysctl"
#define NAME_BUFFER 256

typedef struct {
	const char *name;
	int args;
	int (*handler)(int, char **);
} command_t;

static const char *unit_state(unit_state_t s)
{
	switch (s) {
	case STATE_STARTING:
		return "starting";
	case STATE_STARTED:
		return "started";
	case STATE_STOPPED:
		return "stopped";
	case STATE_FAILED:
		return "failed";
	default:
		return "<unknown>";
	}
}

static errno_t list_units(int argc, char *argv[])
{
	unit_handle_t *units;
	size_t unit_cnt;

	errno_t rc = sysman_get_units(&units, &unit_cnt);
	if (rc != EOK) {
		return rc;
	}

	for (size_t i = 0; i < unit_cnt; i++) {
		unit_handle_t handle = units[i];
		char name[NAME_BUFFER];
		unit_state_t state;

		rc = sysman_unit_get_name(handle, name, NAME_BUFFER);
		if (rc != EOK)
			goto fail;

		rc = sysman_unit_get_state(handle, &state);
		if (rc != EOK)
			goto fail;

		printf("%-25s\t%s\n", name, unit_state(state));
		continue;

	fail:
		printf(" -- unit skipped due to IPC error (%s) --\n",
		    str_error(rc));
	}

	return 0;
}

static errno_t start(int argc, char *argv[])
{
	unit_handle_t handle;
	char *unit_name = argv[1];

	errno_t rc = sysman_unit_handle(unit_name, &handle);
	if (rc != EOK) {
		printf("Cannot obtain handle for unit '%s' (%s).\n",
		    unit_name, str_error(rc));
		return rc;
	}

	rc = sysman_unit_start(handle, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("Error when starting unit '%s' error (%s).\n",
		    unit_name, str_error(rc));
		return rc;
	}

	return 0;
}

static errno_t stop(int argc, char *argv[])
{
	unit_handle_t handle;
	char *unit_name = argv[1];

	errno_t rc = sysman_unit_handle(unit_name, &handle);
	if (rc != EOK) {
		printf("Cannot obtain handle for unit '%s' (%s).\n",
		    unit_name, str_error(rc));
		return rc;
	}

	rc = sysman_unit_stop(handle, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("Error when stopping unit '%s' error (%s).\n",
		    unit_name, str_error(rc));
		return rc;
	}

	return 0;
}

static errno_t shutdown(int argc, char *argv[])
{
	const int delay = 3;
	printf("Will shutdown in %i seconds...\n", delay);
	fibril_sleep(delay);
	printf("Shutdown now.\n");

	errno_t rc = sysman_shutdown();
	if (rc != EOK) {
		printf("Shutdown request failed: %s.\n", str_error(rc));
	}
	return rc;
}

command_t commands[] = {
	{ "list-units", 0, &list_units },
	{ "start",      1, &start },
	{ "stop",       1, &stop },
	{ "shutdown",   0, &shutdown },
	{ 0 }
};

static void print_syntax(void)
{
	printf("%s commands:\n", NAME);
	for (command_t *it = commands; it->name != NULL; ++it) {
		printf("\t%s", it->name);
		for (int i = 0; i < it->args; ++i) {
			printf(" <arg%i>", i + 1);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		print_syntax();
		return 0;
	}

	for (command_t *it = commands; it->name != NULL; ++it) {
		if (str_cmp(it->name, argv[1])) {
			continue;
		}

		int real_args = argc - 2;
		if (it->args < real_args) {
			printf("%s %s: too many arguments\n", NAME, it->name);
			return 1;
		} else if (it->args > real_args) {
			printf("%s %s: too few arguments\n", NAME, it->name);
			return 1;
		} else {
			return it->handler(real_args + 1, argv + 1);
		}
	}

	printf("%s: unknown command '%s'\n", NAME, argv[1]);
	return 1;
}

/** @}
 */
