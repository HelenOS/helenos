/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the original program's authors nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"
#include "help.h"
#include "cmds.h"
#include "modules.h"
#include "builtins.h"
#include "errors.h"

static char *cmdname = "help";
extern const char *progname;

#define HELP_IS_MODULE   1
#define HELP_IS_BUILTIN  0
#define HELP_IS_RUBBISH  -1

volatile int mod_switch = -1;

/* Just use a pointer here, no need for mod_switch */
static int is_mod_or_builtin(char *cmd)
{
	int rc = HELP_IS_RUBBISH;

	rc = is_builtin(cmd);
	if (rc > -1) {
		mod_switch = rc;
		return HELP_IS_BUILTIN;
	}
	rc = is_module(cmd);
	if (rc > -1) {
		mod_switch = rc;
		return HELP_IS_MODULE;
	}

	return HELP_IS_RUBBISH;
}

void *help_cmd_help(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf(
		"\n  %s [command] <extended>\n"
		"  Use help [command] extended for detailed help on [command] "
		", even `help'\n\n", cmdname);
	} else {
		printf(
		"\n  `%s' - shows help for commands\n"
		"  Examples:\n"
		"   %s [command]           Show help for [command]\n"
		"   %s [command] extended  Show extended help for [command]\n"
		"\n  If no argument is given to %s, a list of commands are shown\n\n",
		cmdname, cmdname, cmdname, cmdname);
	}

	return CMD_VOID;
}

int *cmd_help(char *argv[])
{
	module_t *mod;
	builtin_t *cmd;
	unsigned int i = 0;
	int rc = 0;
	int argc;
	int level = HELP_SHORT;

	for (argc = 0; argv[argc] != NULL; argc ++);

	if (argc > 3) {
		printf("\nToo many arguments to `%s', try:\n", cmdname);
		help_cmd_help(HELP_SHORT);
		return CMD_FAILURE;
	}

	if (argc == 3) {
		if (!strcmp("extended", argv[2]))
			level = HELP_LONG;
		else
			level = HELP_SHORT;
	}

	if (argc > 1) {
		rc = is_mod_or_builtin(argv[1]);
		switch (rc) {
		case HELP_IS_RUBBISH:
			printf("Invalid command %s\n", argv[1]);
			return CMD_FAILURE;
		case HELP_IS_MODULE:
			help_module(mod_switch, level);
			return CMD_SUCCESS;
		case HELP_IS_BUILTIN:
			help_builtin(mod_switch, level);
			return CMD_SUCCESS;
		}
	}

	printf("\n  Available commands are:\n");
	printf("  ------------------------------------------------------------\n");

	/* First, show a list of built in commands that are available in this mode */
	for (cmd = builtins; cmd->name != NULL; cmd++, i++) {
		if (!builtin_is_restricted(i)) {
			if (is_builtin_alias(cmd->name))
				printf("   %-16s\tAlias for `%s'\n", cmd->name,
					alias_for_builtin(cmd->name));
			else
				printf("   %-16s\t%s\n", cmd->name, cmd->desc);
		}
	}

	i = 0;

	/* Now, show a list of module commands that are available in this mode */
	for (mod = modules; mod->name != NULL; mod++, i++) {
		if (!module_is_restricted(i)) {
			if (is_module_alias(mod->name))
				printf("   %-16s\tAlias for `%s'\n", mod->name,
					alias_for_module(mod->name));
			else
				printf("   %-16s\t%s\n", mod->name, mod->desc);
		}
	}

	printf("\n  Try %s %s for more information on how `%s' works.\n\n",
		cmdname, cmdname, cmdname);

	return CMD_SUCCESS;
}
