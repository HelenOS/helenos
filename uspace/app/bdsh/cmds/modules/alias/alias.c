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
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "alias.h"
#include "cmds.h"

#include <adt/odict.h>

static const char *cmdname = "alias";
static const char *alias_format = "%s='%s'\n";

static void list_aliases()
{
	odlink_t *alias_link = odict_first(&alias_dict);
	while (alias_link != NULL) {
		alias_t *data = odict_get_instance(alias_link, alias_t, odict);
		printf(alias_format, data->name, data->value);
		alias_link = odict_next(alias_link, &alias_dict);
	}
}

static bool print_alias(const char *name)
{
	odlink_t *alias_link = odict_find_eq(&alias_dict, (void *)name, NULL);
	if (alias_link != NULL) {
		alias_t *data = odict_get_instance(alias_link, alias_t, odict);
		printf(alias_format, data->name, data->value);
		return true;
	}

	cli_error(CL_ENOENT, "%s: No alias with the name '%s' exists\n", cmdname, name);
	return false;
}

static errno_t set_alias(const char *name, const char *value)
{
	odlink_t *alias_link = odict_find_eq(&alias_dict, (void *)name, NULL);

	if (alias_link != NULL) {
		/* update existing value */
		alias_t *data = odict_get_instance(alias_link, alias_t, odict);
		char *dup_value = str_dup(value);

		if (dup_value == NULL) {
			cli_error(CL_ENOMEM, "%s: failing to allocate memory for value\n", cmdname);
			return ENOMEM;
		}

		free(data->value);
		data->value = dup_value;
	} else {
		/* add new value */
		alias_t *data = (alias_t *)calloc(1, sizeof(alias_t));
		if (data == NULL) {
			cli_error(CL_ENOMEM, "%s: failing to allocate memory for data container\n", cmdname);
			return ENOMEM;
		}

		data->name = str_dup(name);
		if (data->name == NULL) {
			cli_error(CL_ENOMEM, "%s: failing to allocate memory for name\n", cmdname);
			free(data);
			return ENOMEM;
		}

		data->value = str_dup(value);
		if (data->value == NULL) {
			cli_error(CL_ENOMEM, "%s: failing to allocate memory for value\n", cmdname);
			free(data->name);
			free(data);
			return ENOMEM;
		}
		odict_insert(&data->odict, &alias_dict, NULL);
	}

	return EOK;
}

static bool validate_name(const char *name)
{
	while (*name != '\0') {
		if (*name == '/')
			return false;
		if (*name == ' ')
			return false;
		if (*name == '\"')
			return false;
		if (*name == '\'')
			return false;
		if (*name == '|')
			return false;

		name++;
	}

	return true;
}

/* Dispays help for alias in various levels */
void help_cmd_alias(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' sets an alias, displays an alias or lists all aliases\n", cmdname);
	} else {
		help_cmd_alias(HELP_SHORT);
		printf("Usage: `%s' [newalias[='existingCMD --flags] ...]'\n\n"
		    "If no parameters are given it will display all existing aliases.\n"
		    "If a parameter without an assignment is given, the value of the given alias will be returned.\n"
		    "If a parameter with an assignment is given, the alias will be created or updated for the given value. "
		    "It is possible to create an alias to a different alias. A circularity will prevent an alias to be resolved.\n",
		    cmdname);
	}
}

/* Main entry point for alias, accepts an array of arguments */
int cmd_alias(char **argv)
{

	if (argv[1] == NULL) {
		list_aliases();
		return CMD_SUCCESS;
	}

	size_t i;
	for (i = 1; argv[i] != NULL; i++) {
		char *name = argv[i];
		char *value;
		if ((value = str_chr(name, '=')) != NULL) {
			name[value - name] = '\0';

			if (!validate_name(name)) {
				cli_error(CL_EFAIL, "%s: invalid alias name given\n", cmdname);
				return CMD_FAILURE;
			}

			if (set_alias(name, value + 1) != EOK) {
				return CMD_FAILURE;
			}
		} else {
			if (!print_alias(name)) {
				return CMD_FAILURE;
			}
		}
	}

	return CMD_SUCCESS;
}
