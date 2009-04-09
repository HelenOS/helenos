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

/* Almost identical (for now) to mod_cmds.c , however this will not be the case
 * soon as builtin_t is going to grow way beyond module_t */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "cmds.h"
#include "builtin_aliases.h"

extern volatile unsigned int cli_interactive;

int is_builtin(const char *command)
{
	builtin_t *cmd;
	unsigned int i = 0;

	if (NULL == command)
		return -2;

	for (cmd = builtins; cmd->name != NULL; cmd++, i++) {
		if (!str_cmp(cmd->name, command))
			return i;
	}

	return -1;
}

int is_builtin_alias(const char *command)
{
	unsigned int i = 0;

	if (NULL == command)
		return -1;

	for(i=0; builtin_aliases[i] != NULL; i+=2) {
		if (!str_cmp(builtin_aliases[i], command))
			return 1;
	}

	return 0;
}

char *alias_for_builtin(const char *command)
{
	unsigned int i = 0;

	if (NULL == command)
		return (char *)NULL;

	for(i=0; builtin_aliases[i] != NULL; i++) {
		if (!str_cmp(builtin_aliases[i], command))
			return (char *)builtin_aliases[++i];
		i++;
	}

	return (char *)NULL;
}

int help_builtin(int builtin, unsigned int extended)
{
	builtin_t *cmd = builtins;

	cmd += builtin;

	if (NULL != cmd->help) {
		cmd->help(extended);
		return CL_EOK;
	} else
		return CL_ENOENT;
}

int run_builtin(int builtin, char *argv[], cliuser_t *usr)
{
	builtin_t *cmd = builtins;

	cmd += builtin;

	if (NULL != cmd->entry)
		return((int)cmd->entry(argv, usr));

	return CL_ENOENT;
}
