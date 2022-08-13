/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <str.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "batch.h"
#include "cmds.h"
#include "input.h"

static const char *cmdname = "batch";

/* Dispays help for batch in various levels */
void help_cmd_batch(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf(
		    "\n  batch [filename] [-c]\n"
		    "  Issues commands stored in the file.\n"
		    "  Each command must correspond to the single line in the file.\n\n");
	} else {
		printf(
		    "\n  `batch' - issues a batch of commands\n"
		    "  Issues commands stored in the file. Each command must correspond\n"
		    "  to the single line in the file. Empty lines can be used to visually\n"
		    "  separate groups of commands. There is no support for comments,\n"
		    "  variables, recursion or other programming constructs - the `batch'\n"
		    "  command is indeed very trivial.\n"
		    "  If the filename is followed by -c, execution continues even if some\n"
		    "  of the commands failed.\n\n");
	}

	return;
}

/** Main entry point for batch, accepts an array of arguments and a
 * pointer to the cliuser_t structure
 */
int cmd_batch(char **argv, cliuser_t *usr)
{
	unsigned int argc;
	bool continue_despite_errors = false;

	/* Count the arguments */
	argc = 0;
	while (argv[argc] != NULL)
		argc++;

	if (argc < 2) {
		printf("%s - no input file provided.\n", cmdname);
		return CMD_FAILURE;
	}

	if (argc > 2) {
		if (str_cmp(argv[2], "-c") == 0)
			continue_despite_errors = true;
	}

	errno_t rc = EOK;
	FILE *batch = fopen(argv[1], "r");
	if (batch == NULL) {
		printf("%s - Cannot open file %s\n", cmdname, argv[1]);
		return CMD_FAILURE;
	} else {
		cliuser_t fusr;
		fusr.name = usr->name;
		fusr.cwd = usr->cwd;
		fusr.prompt = usr->prompt;
		fusr.line = malloc(INPUT_MAX + 1);
		char *cur = fusr.line;
		char *end = fusr.line + INPUT_MAX;
		int c = fgetc(batch);
		while (fusr.line != NULL) {
			if (c == '\n' || c == EOF || cur == end) {
				*cur = '\0';
				if (cur == fusr.line) {
					/* skip empty line */
					rc = EOK;
					free(cur);
				} else {
					printf(">%s\n", fusr.line);
					rc = process_input(&fusr);
					/* fusr->line was freed by process_input() */
					if ((rc != EOK) && continue_despite_errors) {
						/* Mute the error. */
						rc = EOK;
					}
				}
				if (rc == 0 && c != EOF) {
					fusr.line = malloc(INPUT_MAX + 1);
					cur = fusr.line;
					end = fusr.line + INPUT_MAX;
				} else {
					break;
				}
			} else {
				*cur++ = c;
			}
			c = fgetc(batch);
		}
		fclose(batch);
	}

	return CMD_SUCCESS;
}
