/*
 * Copyright (c) 2018 Matthieu Riolo
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
