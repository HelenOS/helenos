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
#include <unistd.h>
#include "config.h"
#include "scli.h"
#include "input.h"
#include "util.h"
#include "errors.h"
#include "cmds/cmds.h"

/* See scli.h */
static cliuser_t usr;

/* Modified by the 'quit' module, which is compiled before this */
extern unsigned int cli_quit;

/* Globals that are modified during start-up that modules/builtins should
 * be aware of. */
volatile unsigned int cli_interactive = 1;
volatile unsigned int cli_verbocity = 1;

/* The official name of this program
 * (change to your liking in configure.ac and re-run autoconf) */
const char *progname = PACKAGE_NAME;

/* (re)allocates memory to store the current working directory, gets
 * and updates the current working directory, formats the prompt string */
unsigned int cli_set_prompt(cliuser_t *usr)
{
	usr->prompt = (char *) realloc(usr->prompt, PATH_MAX);
	if (NULL == usr->prompt) {
		cli_error(CL_ENOMEM, "Can not allocate prompt");
		return 1;
	}
	memset(usr->prompt, 0, sizeof(usr->prompt));

	usr->cwd = (char *) realloc(usr->cwd, PATH_MAX);
	if (NULL == usr->cwd) {
		cli_error(CL_ENOMEM, "Can not allocate cwd");
		return 1;
	}
	memset(usr->cwd, 0, sizeof(usr->cwd));

	usr->cwd = getcwd(usr->cwd, PATH_MAX - 1);

	if (NULL == usr->cwd)
		snprintf(usr->cwd, PATH_MAX, "(unknown)");

	snprintf(usr->prompt,
			PATH_MAX,
			"%s # ",
			usr->cwd);

	return 0;
}

int cli_init(cliuser_t *usr)
{
	usr->line = (char *) NULL;
	usr->name = "root";
	usr->home = "/";
	usr->cwd = (char *) NULL;
	usr->prompt = (char *) NULL;
	chdir(usr->home);
	usr->lasterr = 0;
	return (int) cli_set_prompt(usr);
}

/* Destructor */
void cli_finit(cliuser_t *usr)
{
	if (NULL != usr->line)
		free(usr->line);
	if (NULL != usr->prompt)
		free(usr->prompt);
	if (NULL != usr->cwd)
		free(usr->cwd);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int i = 0;

	if (cli_init(&usr))
		exit(EXIT_FAILURE);

	printf("Welcome to %s - %s\nType `help' at any time for usage information.\n",
		progname, PACKAGE_STRING);

	while (!cli_quit) {
		cli_set_prompt(&usr);
		get_input(&usr);
		if (NULL != usr.line) {
			ret = tok_input(&usr);
			usr.lasterr = ret;
		}
		i++;
	}
	goto finit;

finit:
	cli_finit(&usr);
	return ret;
}
