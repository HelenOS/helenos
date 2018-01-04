/*
 * Copyright (c) 2008 Tim Post
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
#include <stddef.h>
#include <str.h>
#include "config.h"
#include "scli.h"
#include "input.h"
#include "util.h"
#include "errors.h"
#include "cmds/cmds.h"

/* See scli.h */
static cliuser_t usr;
static iostate_t *iostate;
static iostate_t stdiostate;

/* Globals that are modified during start-up that modules/builtins
 * should be aware of. */
volatile unsigned int cli_quit = 0;
volatile unsigned int cli_verbocity = 1;

/* The official name of this program
 * (change to your liking in configure.ac and re-run autoconf) */
const char *progname = PACKAGE_NAME;

/* These are not exposed, even to builtins */
static int cli_init(cliuser_t *);
static void cli_finit(cliuser_t *);

/* Constructor */
static int cli_init(cliuser_t *usr)
{
	usr->line = (char *) NULL;
	usr->name = "root";
	usr->cwd = (char *) NULL;
	usr->prompt = (char *) NULL;
	usr->lasterr = 0;

	if (input_init() != 0)
		return 1;

	return (int) cli_set_prompt(usr);
}

/* Destructor */
static void cli_finit(cliuser_t *usr)
{
	if (NULL != usr->line)
		free(usr->line);
	if (NULL != usr->prompt)
		free(usr->prompt);
	if (NULL != usr->cwd)
		free(usr->cwd);
}

iostate_t *get_iostate(void)
{
	return iostate;
}


void set_iostate(iostate_t *ios)
{
	iostate = ios;
	stdin = ios->stdin;
	stdout = ios->stdout;
	stderr = ios->stderr;
}

int main(int argc, char *argv[])
{
	errno_t ret = 0;
	
	stdiostate.stdin = stdin;
	stdiostate.stdout = stdout;
	stdiostate.stderr = stderr;
	iostate = &stdiostate;

	if (cli_init(&usr))
		exit(EXIT_FAILURE);

	while (!cli_quit) {
		get_input(&usr);
		if (NULL != usr.line) {
			ret = process_input(&usr);
			cli_set_prompt(&usr);
			usr.lasterr = ret;
		}
	}

	printf("Leaving %s.\n", progname);

	cli_finit(&usr);
	return ret;
}
