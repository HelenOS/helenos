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
#include <str.h>
#include <io/console.h>
#include <io/keycode.h>
#include <io/style.h>
#include <io/color.h>
#include <vfs/vfs.h>
#include <clipboard.h>
#include <macros.h>
#include <errno.h>
#include <assert.h>
#include <bool.h>
#include <tinput.h>

#include "config.h"
#include "util.h"
#include "scli.h"
#include "input.h"
#include "errors.h"
#include "exec.h"
#include "tok.h"

extern volatile unsigned int cli_quit;

/** Text input field. */
static tinput_t *tinput;

/* Private helpers */
static int run_command(char **, cliuser_t *);

/* Tokenizes input from console, sees if the first word is a built-in, if so
 * invokes the built-in entry point (a[0]) passing all arguments in a[] to
 * the handler */
int tok_input(cliuser_t *usr)
{
	char *cmd[WORD_MAX];
	int rc = 0;
	tokenizer_t tok;

	if (NULL == usr->line)
		return CL_EFAIL;

	rc = tok_init(&tok, usr->line, cmd, WORD_MAX);
	if (rc != EOK) {
		goto finit;
	}
	
	rc = tok_tokenize(&tok);
	if (rc != EOK) {
		goto finit;
	}
	
	rc = run_command(cmd, usr);
	
finit:
	if (NULL != usr->line) {
		free(usr->line);
		usr->line = (char *) NULL;
	}
	tok_fini(&tok);

	return rc;
}

int run_command(char **cmd, cliuser_t *usr)
{
	int id = 0;
	
	/* We have rubbish */
	if (NULL == cmd[0]) {
		return CL_ENOENT;
	}
	
	/* Is it a builtin command ? */
	if ((id = (is_builtin(cmd[0]))) > -1) {
		return run_builtin(id, cmd, usr);
	}
	
	/* Is it a module ? */
	if ((id = (is_module(cmd[0]))) > -1) {
		return run_module(id, cmd);
	}

	/* See what try_exec thinks of it */
	return try_exec(cmd[0], cmd);
}

void get_input(cliuser_t *usr)
{
	char *str;
	int rc;
	
	console_flush(tinput->console);
	console_set_style(tinput->console, STYLE_EMPHASIS);
	printf("%s", usr->prompt);
	console_flush(tinput->console);
	console_set_style(tinput->console, STYLE_NORMAL);

	rc = tinput_read(tinput, &str);
	if (rc == ENOENT) {
		/* User requested exit */
		cli_quit = 1;
		putchar('\n');
		return;
	}

	if (rc != EOK) {
		/* Error in communication with console */
		return;
	}

	/* Check for empty input. */
	if (str_cmp(str, "") == 0) {
		free(str);
		return;
	}

	usr->line = str;
	return;
}

int input_init(void)
{
	tinput = tinput_new();
	if (tinput == NULL) {
		printf("Failed to initialize input.\n");
		return 1;
	}

	return 0;
}
