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

	printf("%s: No alias with the name '%s' exists\n", cmdname, name);
	return false;
}

static void set_alias(const char *name, const char *value)
{
	odlink_t *alias_link = odict_find_eq(&alias_dict, (void *)name, NULL);

	if (alias_link != NULL) {
		//update existing value
		alias_t *data = odict_get_instance(alias_link, alias_t, odict);
		free(data->value);
		data->value = str_dup(value);
	} else {
		//add new value
		alias_t *data = (alias_t *)calloc(1, sizeof(alias_t));
		data->name = str_dup(name);
		data->value = str_dup(value);

		odict_insert(&data->odict, &alias_dict, NULL);
	}
}

/* Dispays help for alias in various levels */
void help_cmd_alias(unsigned int level)
{
	printf("`%s' sets an alias, displays an alias or lists all aliases\n", cmdname);
	return;
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
		char *name = str_dup(argv[i]);
		char *value;
		if ((value = str_chr(name, '=')) != NULL) {
			name[value - name] = '\0';
			set_alias(name, value + 1);
		} else {
			if (!print_alias(name)) {
				free(name);
				return CMD_FAILURE;
			}
		}

		free(name);
	}
	
	return CMD_SUCCESS;
}
