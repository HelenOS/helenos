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
#include "commands.h"

#define NAME   "tmon"
#define INDENT "      "

typedef struct tmon_cmd {
	const char *name;
	const char *description;
	int (*action)(int, char **);
} tmon_cmd_t;

static tmon_cmd_t commands[] = {
	{
		.name = "list",
		.description = "Print a list of connected diagnostic devices.",
		.action = tmon_list,
	},
	{
		.name = "test-intr-in",
		.description = "Read from interrupt endpoint as fast as possible.",
		.action = tmon_burst_intr_in,
	},
	{
		.name = "test-intr-out",
		.description = "Write to interrupt endpoint as fast as possible.",
		.action = tmon_burst_intr_out,
	},
	{
		.name = "test-bulk-in",
		.description = "Read from bulk endpoint as fast as possible.",
		.action = tmon_burst_bulk_in,
	},
	{
		.name = "test-bulk-out",
		.description = "Write to bulk endpoint as fast as possible.",
		.action = tmon_burst_bulk_out,
	},
	{
		.name = "test-isoch-in",
		.description = "Read from isochronous endpoint as fast as possible.",
		.action = tmon_burst_isoch_in,
	},
	{
		.name = "test-isoch-out",
		.description = "Write to isochronous endpoint as fast as possible.",
		.action = tmon_burst_isoch_out,
	},
	{ /* NULL-terminated */ }
};

typedef struct tmon_opt {
	const char *long_name;
	char short_name;
	const char *description;
} tmon_opt_t;

static tmon_opt_t options[] = {
	{
		.long_name = "cycles",
		.short_name = 'n',
		.description = "Set the number of read/write cycles."
	},
	{
		.long_name = "size",
		.short_name = 's',
		.description = "Set the data size transferred in a single cycle."
	},
	{ /* NULL-terminated */ }
};

static void print_usage(char *app_name)
{
	puts(NAME ": benchmark USB diagnostic device\n\n");
	printf("Usage: %s command [device] [options]\n\n", app_name);

	for (int i = 0; commands[i].name; ++i) {
		printf(INDENT "%s - %s\n", commands[i].name, commands[i].description);
	}

	puts("\n");
	for (int i = 0; options[i].long_name; ++i) {
		printf(INDENT "-%c --%s\n" INDENT INDENT "%s\n", options[i].short_name, options[i].long_name, options[i].description);
	}

	puts("\nIf no device is specified, the first device is used provided that it is the only one connected. Otherwise, the command fails.\n\n");
}

int main(int argc, char *argv[])
{
	// Find a command to execute.
	tmon_cmd_t *cmd = NULL;
	for (int i = 0; argc > 1 && commands[i].name; ++i) {
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
