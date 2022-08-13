/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 * SPDX-FileCopyrightText: 2018 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <adt/odict.h>
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

odict_t alias_dict;

/*
 * Globals that are modified during start-up that modules/builtins
 * should be aware of.
 */
volatile unsigned int cli_quit = 0;
volatile unsigned int cli_verbocity = 1;

/*
 * The official name of this program
 * (change to your liking in configure.ac and re-run autoconf)
 */
const char *progname = PACKAGE_NAME;

static int alias_cmp(void *a, void *b)
{
	return str_cmp((char *)a, (char *)b);
}

static void *alias_key(odlink_t *odlink)
{
	return (void *)odict_get_instance(odlink, alias_t, odict)->name;
}

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

	odict_initialize(&alias_dict, alias_key, alias_cmp);

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
