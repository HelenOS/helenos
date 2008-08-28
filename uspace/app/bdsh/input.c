/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * All rights reserved.
 * Copyright (c) 2008, Jiri Svoboda - All Rights Reserved
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
#include <io/stream.h>

#include "config.h"
#include "util.h"
#include "scli.h"
#include "input.h"
#include "errors.h"
#include "exec.h"

extern volatile unsigned int cli_interactive;

/* Not exposed in input.h */
static void cli_restricted(char *);
static void read_line(char *, int);

/* More than a macro than anything */
static void cli_restricted(char *cmd)
{
	printf("%s is not available in %s mode\n", cmd,
		cli_interactive ? "interactive" : "non-interactive");

	return;
}

/* Tokenizes input from console, sees if the first word is a built-in, if so
 * invokes the built-in entry point (a[0]) passing all arguments in a[] to
 * the handler */
int tok_input(cliuser_t *usr)
{
	char *cmd[WORD_MAX];
	int n = 0, i = 0;
	int rc = 0;
	char *tmp;

	if (NULL == usr->line)
		return CL_EFAIL;

	tmp = cli_strdup(usr->line);

	/* Break up what the user typed, space delimited */

	/* TODO: Protect things in quotes / ticks, expand wildcards */
	cmd[n] = cli_strtok(tmp, " ");
	while (cmd[n] && n < WORD_MAX) {
		cmd[++n] = cli_strtok(NULL, " ");
	}

	/* We have rubbish */
	if (NULL == cmd[0]) {
		rc = CL_ENOENT;
		goto finit;
	}

	/* Its a builtin command */
	if ((i = (is_builtin(cmd[0]))) > -1) {
		/* Its not available in this mode, see what try_exec() thinks */
		if (builtin_is_restricted(i)) {
				rc = try_exec(cmd[0], cmd);
				if (rc)
					/* No external matching it could be found, tell the
					 * user that the command does exist, but is not
					 * available in this mode. */
					cli_restricted(cmd[0]);
				goto finit;
		}
		/* Its a builtin, its available, run it */
		rc = run_builtin(i, cmd, usr);
		goto finit;
	/* We repeat the same dance for modules */
	} else if ((i = (is_module(cmd[0]))) > -1) {
		if (module_is_restricted(i)) {
			rc = try_exec(cmd[0], cmd);
			if (rc)
				cli_restricted(cmd[0]);
			goto finit;
		}
		rc = run_module(i, cmd);
		goto finit;
	} else {
		/* Its not a module or builtin, restricted or otherwise.
		 * See what try_exec() thinks of it and just pass its return
		 * value back to the caller */
		rc = try_exec(cmd[0], cmd);
		goto finit;
	}

finit:
	if (NULL != usr->line) {
		free(usr->line);
		usr->line = (char *) NULL;
	}
	if (NULL != tmp)
		free(tmp);

	return rc;
}

/* Borrowed from Jiri Svoboda's 'cli' uspace app */
static void read_line(char *buffer, int n)
{
	char c;
	int chars;

	chars = 0;
	while (chars < n - 1) {
		c = getchar();
		if (c < 0)
			return;
		if (c == '\n')
			break;
		if (c == '\b') {
			if (chars > 0) {
				putchar('\b');
				--chars;
			}
			continue;
		}
		putchar(c);
		buffer[chars++] = c;
	}
	putchar('\n');
	buffer[chars] = '\0';
}

/* TODO:
 * Implement something like editline() / readline(), if even
 * just for command history and making arrows work. */
void get_input(cliuser_t *usr)
{
	char line[INPUT_MAX];
	size_t len = 0;

	printf("%s", usr->prompt);
	read_line(line, INPUT_MAX);
	len = strlen(line);
	/* Make sure we don't have rubbish or a C/R happy user */
	if (len == 0 || line[0] == '\n')
		return;
	usr->line = cli_strdup(line);

	return;
}

