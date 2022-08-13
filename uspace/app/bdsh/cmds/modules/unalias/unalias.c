/*
 * SPDX-FileCopyrightText: 2018 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <adt/odict.h>

#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "unalias.h"
#include "cmds.h"

static const char *cmdname = "unalias";

static void free_alias(odlink_t *alias_link)
{
	alias_t *data = odict_get_instance(alias_link, alias_t, odict);
	odict_remove(alias_link);

	free(data->name);
	free(data->value);
	free(data);
}

/* Dispays help for unalias in various levels */
void help_cmd_unalias(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' removes an alias or all aliases with -a\n", cmdname);
	} else {
		help_cmd_unalias(HELP_SHORT);
		printf("Usage: `%s' -a'\n"
		    "`%s' name [name ...]'\n\n"
		    "If no parameters are given it will display this help message.\n"
		    "If the flag -a is given, all existing aliases will be removed.\n"
		    "If one or multiple parameters are given, then those aliases will be removed.\n",
		    cmdname, cmdname);
	}
}

/* Main entry point for unalias, accepts an array of arguments */
int cmd_unalias(char **argv)
{

	if (argv[1] == NULL) {
		help_cmd_unalias(HELP_LONG);
		return CMD_SUCCESS;
	}

	odlink_t *alias_link;

	unsigned int argc = cli_count_args(argv);
	if (argc == 2) {
		if (str_cmp(argv[1], "-a") == 0) {
			alias_link = odict_first(&alias_dict);
			while (alias_link != NULL) {
				odlink_t *old_alias_link = alias_link;
				alias_link = odict_next(old_alias_link, &alias_dict);
				free_alias(old_alias_link);
			}

			return CMD_SUCCESS;
		}
	}

	int rc = CMD_SUCCESS;
	size_t i;
	for (i = 1; argv[i] != NULL; i++) {
		alias_link = odict_find_eq(&alias_dict, (void *)argv[i], NULL);

		if (alias_link == NULL) {
			cli_error(CL_ENOENT, "%s: No alias '%s' found\n", cmdname, argv[i]);
			rc = CMD_FAILURE;
		} else {
			free_alias(alias_link);
		}
	}

	return rc;
}
