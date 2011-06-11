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
static int run_command(char **, cliuser_t *, iostate_t *);
static void print_pipe_usage(void);

/* Tokenizes input from console, sees if the first word is a built-in, if so
 * invokes the built-in entry point (a[0]) passing all arguments in a[] to
 * the handler */
int process_input(cliuser_t *usr)
{
	char *cmd[WORD_MAX];
	int rc = 0;
	tokenizer_t tok;
	int i, pipe_count, processed_pipes;
	int pipe_pos[2];
	char **actual_cmd;
	char *redir_from = NULL;
	char *redir_to = NULL;

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
	
	/* Until full support for pipes is implemented, allow for a simple case:
	 * [from <file> |] command [| to <file>]
	 * 
	 * First find the pipes and check that there are no more
	 */
	int cmd_length = 0;
	for (i = 0, pipe_count = 0; cmd[i] != NULL; i++, cmd_length++) {
		if (cmd[i][0] == '|') {
			if (pipe_count >= 2) {
				print_pipe_usage();
				rc = ENOTSUP;
				goto finit;
			}
			pipe_pos[pipe_count] = i;
			pipe_count++;
		}
	}
	
	actual_cmd = cmd;
	processed_pipes = 0;
	
	/* Check if the first part (from <file> |) is present */
	if (pipe_count > 0 && pipe_pos[0] == 2 && str_cmp(cmd[0], "from") == 0) {
		/* Ignore the first three tokens (from, file, pipe) and set from */
		redir_from = cmd[1];
		actual_cmd = cmd + 3;
		processed_pipes++;
	}
	
	/* Check if the second part (| to <file>) is present */
	if ((pipe_count - processed_pipes) > 0 &&
	    pipe_pos[processed_pipes] == cmd_length - 3 &&
	    str_cmp(cmd[cmd_length-2], "to") == 0) {
		/* Ignore the last three tokens (pipe, to, file) and set to */
		redir_to = cmd[cmd_length-1];
		cmd[cmd_length-3] = NULL;
		cmd_length -= 3;
		processed_pipes++;
	}
	
	if (processed_pipes != pipe_count) {
		print_pipe_usage();
		rc = ENOTSUP;
		goto finit;
	}
	
	if (actual_cmd[0] == NULL) {
		print_pipe_usage();
		rc = ENOTSUP;
		goto finit;
	}
	
	iostate_t new_iostate = {
		.stdin = stdin,
		.stdout = stdout,
		.stderr = stderr
	};
	
	FILE *from = NULL;
	FILE *to = NULL;
	
	if (redir_from) {
		from = fopen(redir_from, "r");
		if (from == NULL) {
			printf("Cannot open file %s\n", redir_from);
			rc = errno;
			goto finit_with_files;
		}
		new_iostate.stdin = from;
	}
	
	
	if (redir_to) {
		to = fopen(redir_to, "w");
		if (to == NULL) {
			printf("Cannot open file %s\n", redir_to);
			rc = errno;
			goto finit_with_files;
		}
		new_iostate.stdout = to;
	}
	
	rc = run_command(cmd, usr, &new_iostate);
	
finit_with_files:
	if (from != NULL) {
		fclose(from);
	}
	if (to != NULL) {
		fclose(to);
	}
	
finit:
	if (NULL != usr->line) {
		free(usr->line);
		usr->line = (char *) NULL;
	}
	tok_fini(&tok);

	return rc;
}

void print_pipe_usage()
{
	printf("Invalid syntax!\n");
	printf("Usage of redirection (pipes in the future):\n");
	printf("from filename | command ...\n");
	printf("from filename | command ... | to filename\n");
	printf("command ... | to filename\n");
	
}

int run_command(char **cmd, cliuser_t *usr, iostate_t *new_iostate)
{
	int id = 0;
	
	/* We have rubbish */
	if (NULL == cmd[0]) {
		return CL_ENOENT;
	}
	
	/* Is it a builtin command ? */
	if ((id = (is_builtin(cmd[0]))) > -1) {
		return run_builtin(id, cmd, usr, new_iostate);
	}
	
	/* Is it a module ? */
	if ((id = (is_module(cmd[0]))) > -1) {
		return run_module(id, cmd, new_iostate);
	}

	/* See what try_exec thinks of it */
	return try_exec(cmd[0], cmd, new_iostate);
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
