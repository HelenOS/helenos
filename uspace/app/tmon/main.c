/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup tmon
 * @{
 */
/**
 * @file
 * USB transfer debugging.
 */

#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include <macros.h>
#include "commands.h"

#define NAME   "tmon"
#define INDENT "      "

/** Command which is executed by tmon. */
typedef struct tmon_cmd {
	/** Unique name, by which the command is executed. */
	const char *name;
	/** Description of the command, which is displayed in the usage string. */
	const char *description;
	/** Function, which executes the command. Same as int main(int, char**). */
	int (*action)(int, char **);
} tmon_cmd_t;

/** Static array of commands supported by tmon. */
static tmon_cmd_t commands[] = {
	{
		.name = "list",
		.description = "Print a list of connected diagnostic devices.",
		.action = tmon_list,
	},
	{
		.name = "test-intr-in",
		.description = "Read from interrupt endpoint as fast as possible.",
		.action = tmon_test_intr_in,
	},
	{
		.name = "test-intr-out",
		.description = "Write to interrupt endpoint as fast as possible.",
		.action = tmon_test_intr_out,
	},
	{
		.name = "test-bulk-in",
		.description = "Read from bulk endpoint as fast as possible.",
		.action = tmon_test_bulk_in,
	},
	{
		.name = "test-bulk-out",
		.description = "Write to bulk endpoint as fast as possible.",
		.action = tmon_test_bulk_out,
	},
	{
		.name = "test-isoch-in",
		.description = "Read from isochronous endpoint as fast as possible.",
		.action = tmon_test_isoch_in,
	},
	{
		.name = "test-isoch-out",
		.description = "Write to isochronous endpoint as fast as possible.",
		.action = tmon_test_isoch_out,
	},
};

/** Option shown in the usage string. */
typedef struct tmon_opt {
	/** Long name of the option without "--" prefix. */
	const char *long_name;
	/** Short name of the option without "-" prefix. */
	char short_name;
	/** Description of the option displayed in the usage string. */
	const char *description;
} tmon_opt_t;

/** Static array of options displayed in the tmon usage string. */
static tmon_opt_t options[] = {
	{
		.long_name = "duration",
		.short_name = 't',
		.description = "Set the minimum test duration (in seconds)."
	},
	{
		.long_name = "size",
		.short_name = 's',
		.description = "Set the data size (in bytes) transferred in a single cycle."
	},
	{
		.long_name = "validate",
		.short_name = 'v',
		.description = "Validate the correctness of transferred data (impacts performance)."
	},
};

/** Print usage string.
 * @param[in] app_name Name to print in the invocation example.
 */
static void print_usage(char *app_name)
{
	puts(NAME ": benchmark USB diagnostic device\n");
	printf("Usage: %s command [device] [options]\n\n", app_name);

	for (unsigned i = 0; i < ARRAY_SIZE(commands); ++i) {
		printf(INDENT "%s - %s\n", commands[i].name, commands[i].description);
	}

	puts("");
	for (unsigned i = 0; i < ARRAY_SIZE(options); ++i) {
		printf(INDENT "-%c --%s\n" INDENT INDENT "%s\n", options[i].short_name, options[i].long_name, options[i].description);
	}

	puts("\nIf no device is specified, the first device is used provided that it is the only one connected. Otherwise, the command fails.\n");
}

/** Main tmon entry point.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int main(int argc, char *argv[])
{
	// Find a command to execute.
	tmon_cmd_t *cmd = NULL;
	for (unsigned i = 0; argc > 1 && i < ARRAY_SIZE(commands); ++i) {
		if (str_cmp(argv[1], commands[i].name) == 0) {
			cmd = commands + i;
			break;
		}
	}

	if (!cmd) {
		print_usage(argv[0]);
		return -1;
	}

	return cmd->action(argc - 1, argv + 1);
}

/** @}
 */
