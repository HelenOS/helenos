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

#define NAME "tmon"

typedef struct {
	const char *name;
	const char *description;
	int (*action)(int, char **);
} usb_diag_cmd_t;

static usb_diag_cmd_t commands[] = {
	{
		.name = "list",
		.description = "Print a list of connected diagnostic devices.",
		.action = tmon_list,
	},
	{
		.name = "test-intr-in",
		.description = "Read from interrupt in endpoints as fast as possible.",
		.action = tmon_burst_intr_in,
	},
	{
		.name = "test-intr-out",
		.description = "Write to interrupt out endpoints as fast as possible.",
		.action = tmon_burst_intr_out,
	},
	{
		.name = "test-bulk-in",
		.description = "Read from bulk in endpoints as fast as possible.",
		.action = tmon_burst_bulk_in,
	},
	{
		.name = "test-bulk-out",
		.description = "Write to bulk out endpoints as fast as possible.",
		.action = tmon_burst_bulk_out,
	},
	{
		.name = "test-isoch-in",
		.description = "Read from isochronous in endpoints as fast as possible.",
		.action = tmon_burst_isoch_in,
	},
	{
		.name = "test-isoch-out",
		.description = "Write to isochronouse out endpoints as fast as possible.",
		.action = tmon_burst_isoch_out,
	},
	{
		.name = NULL
	}
};

static void print_usage(char *app_name)
{
	printf(NAME ": benchmark USB diagnostic device\n\n");

	printf("Usage: %s command [device] [options]\n", app_name);
	printf("Available commands:\n");
	for (int i = 0; commands[i].name; ++i) {
		printf("      %s - %s\n", commands[i].name, commands[i].description);
	}

	// TODO: Print options.

	printf("\nIf no device is specified, the first device is used provided that no other device is connected.\n\n");
}

int main(int argc, char *argv[])
{
	// Find a command to execute.
	usb_diag_cmd_t *cmd = NULL;
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
