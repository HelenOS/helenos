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
#include <str.h>
#include <errno.h>
#include <vfs/vfs.h>

#include "util.h"
#include "errors.h"
#include "entry.h"
#include "cmds.h"
#include "cd.h"

static const char *cmdname = "cd";

/* Previous directory variables.
 *
 * Declaring them static to avoid many "== NULL" checks.
 * PATH_MAX is not that big to cause any problems with memory overhead.
 */
static char previous_directory[PATH_MAX] = "";
static char previous_directory_tmp[PATH_MAX];
static bool previous_directory_valid = true;
static bool previous_directory_set = false;

static errno_t chdir_and_remember(const char *new_dir)
{

	errno_t rc = vfs_cwd_get(previous_directory_tmp, PATH_MAX);
	previous_directory_valid = (rc == EOK);
	previous_directory_set = true;

	rc = vfs_cwd_set(new_dir);
	if (rc != EOK)
		return rc;

	str_cpy(previous_directory, PATH_MAX, previous_directory_tmp);
	return EOK;
}

void help_cmd_cd(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' changes the current working directory.\n", cmdname);
	} else {
		printf(
		"  %s <directory>\n"
		"  Change directory to <directory>, e.g `%s /sbin'\n",
			cmdname, cmdname);
	}

	return;
}


/* This is a very rudamentary 'cd' command. It is not 'link smart' (yet) */

int cmd_cd(char **argv, cliuser_t *usr)
{
	int argc;
	errno_t rc = EOK;

	argc = cli_count_args(argv);

	/* Handle cd -- -. Override to switch to a directory named '-' */
	bool hyphen_override = false;
	char *target_directory = argv[1];
	if (argc == 3) {
		if (!str_cmp(argv[1], "--")) {
			hyphen_override = true;
			argc--;
			target_directory = argv[2];
		}
	}

	/* We don't yet play nice with whitespace, a getopt implementation should
	 * protect "quoted\ destination" as a single argument. Its not our job to
	 * look for && || or redirection as the tokenizer should have done that
	 * (currently, it does not) */

	if (argc > 2) {
		cli_error(CL_EFAIL, "Too many arguments to `%s'", cmdname);
		return CMD_FAILURE;
	}

	if (argc < 2) {
		printf("%s - no directory specified. Try `help %s extended'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	/* We have the correct # of arguments */
	// TODO: handle tidle (~) expansion? */

	/* Handle 'cd -' first. */
	if (!str_cmp(target_directory, "-") && !hyphen_override) {
		if (!previous_directory_valid) {
			cli_error(CL_EFAIL, "Cannot switch to previous directory");
			return CMD_FAILURE;
		}
		if (!previous_directory_set) {
			cli_error(CL_EFAIL, "No previous directory to switch to");
			return CMD_FAILURE;
		}
		char *prev_dup = str_dup(previous_directory);
		if (prev_dup == NULL) {
			cli_error(CL_ENOMEM, "Cannot switch to previous directory");
			return CMD_FAILURE;
		}
		rc = chdir_and_remember(prev_dup);
		free(prev_dup);
	} else {
		rc = chdir_and_remember(target_directory);
	}

	if (rc == 0) {
		cli_set_prompt(usr);
		return CMD_SUCCESS;
	} else {
		switch (rc) {
		case ENOMEM:
			cli_error(CL_EFAIL, "Destination path too long");
			break;
		case ENOENT:
			cli_error(CL_ENOENT, "Invalid directory `%s'", target_directory);
			break;
		default:
			cli_error(CL_EFAIL, "Unable to change to `%s'", target_directory);
			break;
		}
	}

	return CMD_FAILURE;
}
