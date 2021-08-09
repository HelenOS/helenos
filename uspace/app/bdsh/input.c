/*
 * Copyright (c) 2008 Tim Post
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2011 Martin Sucha
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
#include <io/console.h>
#include <io/keycode.h>
#include <io/style.h>
#include <io/color.h>
#include <vfs/vfs.h>
#include <clipboard.h>
#include <macros.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <tinput.h>
#include <adt/odict.h>
#include <adt/list.h>

#include "config.h"
#include "compl.h"
#include "util.h"
#include "scli.h"
#include "input.h"
#include "errors.h"
#include "exec.h"
#include "tok.h"

#define MAX_PIPES 10U

extern volatile unsigned int cli_quit;

/** Text input field. */
static tinput_t *tinput;

/* Private helpers */
static int run_command(char **, cliuser_t *, iostate_t *);

typedef struct {
	link_t alias_hup_link;
	alias_t *alias;
} alias_hup_t;
#if 0
static bool find_alias_hup(alias_t *alias, list_t *alias_hups)
{
	list_foreach(*alias_hups, alias_hup_link, alias_hup_t, link) {
		if (alias == link->alias) {
			return true;
		}
	}

	return false;
}
#endif
/*
 * Tokenizes input from console, sees if the first word is a built-in, if so
 * invokes the built-in entry point (a[0]) passing all arguments in a[] to
 * the handler
 */
static errno_t process_input_nohup(cliuser_t *usr, list_t *alias_hups, size_t count_executed_hups)
{
	char *cmd[WORD_MAX];
	size_t cmd_argc = 0;
	errno_t rc = EOK;
	tokenizer_t tok;
	unsigned int i, pipe_count;
	unsigned int pipe_pos[MAX_PIPES];
	char *redir_from = NULL;
	char *redir_to = NULL;

	if (count_executed_hups >= HUBS_MAX) {
		cli_error(CL_EFAIL, "%s: maximal alias hubs reached\n", PACKAGE_NAME);
		return ELIMIT;
	}

	token_t *tokens_buf = calloc(WORD_MAX, sizeof(token_t));
	if (tokens_buf == NULL)
		return ENOMEM;
	token_t *tokens = tokens_buf;

	if (usr->line == NULL) {
		free(tokens_buf);
		return EINVAL;
	}

	rc = tok_init(&tok, usr->line, tokens, WORD_MAX);
	if (rc != EOK) {
		goto finit;
	}

	size_t tokens_length;
	rc = tok_tokenize(&tok, &tokens_length);
	if (rc != EOK) {
		goto finit;
	}

	if (tokens_length > 0 && tokens[0].type == TOKTYPE_SPACE) {
		tokens++;
		tokens_length--;
	}

	if (tokens_length > 0 && tokens[tokens_length - 1].type == TOKTYPE_SPACE) {
		tokens_length--;
	}

	cmd_argc = tokens_length;
	for (i = 0, pipe_count = 0; i < tokens_length; i++) {
		switch (tokens[i].type) {
		case  TOKTYPE_PIPE:
			pipe_pos[pipe_count++] = i;
			cmd_argc = i;
			redir_to = (char *)"/tmp/pipe";
			break;

		case TOKTYPE_RDIN:
			redir_from = tokens[i + 1].text;
			cmd_argc = i;
			break;

		case TOKTYPE_RDOU:
			redir_to = tokens[i + 1].text;
			cmd_argc = i;
			break;

		default:
			break;
		}
		if (pipe_count > MAX_PIPES) {
			rc = ENOTSUP;
			goto finit;
		}
	}

	unsigned int cmd_token_start = 0;
	unsigned int cmd_token_end = cmd_argc;

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

	for (unsigned p = 0; p < pipe_count; p++) {
		/* Convert tokens of the command to string array */
		unsigned int cmd_pos = 0;
		for (i = cmd_token_start; i < cmd_token_end; i++) {
			if (tokens[i].type != TOKTYPE_SPACE) {
				cmd[cmd_pos++] = tokens[i].text;
			}
		}
		cmd[cmd_pos++] = NULL;

		if (cmd[0] == NULL) {
			printf("Command not found.\n");
			rc = ENOTSUP;
			goto finit;
		}

		if (p < pipe_count - 1) {
			new_iostate.stdout = to;
		} else {
			new_iostate.stdin = to;
		}

		if (run_command(cmd, usr, &new_iostate) == 0) {
			rc = EOK;
		} else {
			rc = EINVAL;
		}

		// Restore the Standard Input, Output and Error file descriptors
		new_iostate.stdin = stdin;
		new_iostate.stdout = stdout;
		new_iostate.stderr = stderr;

		cmd_token_start = cmd_token_end + 1;
		cmd_token_end = (p < pipe_count - 1) ? pipe_pos[p + 1] : tokens_length;
	}

	unsigned int cmd_pos = 0;
	for (i = cmd_token_start; i < cmd_token_end; i++) {
		if (tokens[i].type != TOKTYPE_SPACE) {
			cmd[cmd_pos++] = tokens[i].text;
		}
	}
	cmd[cmd_pos++] = NULL;

	if (cmd[0] == NULL) {
		printf("Command not found.\n");
		rc = ENOTSUP;
		goto finit;
	}

	if (pipe_count) {
		fseek(to, 0, SEEK_SET);
		new_iostate.stdin = to;
	}


	if (run_command(cmd, usr, &new_iostate) == 0) {
		rc = EOK;
	} else {
		rc = EINVAL;
	}

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
	free(tokens_buf);

	return rc;
}

errno_t process_input(cliuser_t *usr)
{
	list_t alias_hups;
	list_initialize(&alias_hups);

	errno_t rc = process_input_nohup(usr, &alias_hups, 0);

	list_foreach_safe(alias_hups, cur_link, next_link) {
		alias_hup_t *cur_item = list_get_instance(cur_link, alias_hup_t, alias_hup_link);
		free(cur_item);
	}

	return rc;
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
	errno_t rc;

	tinput_set_prompt(tinput, usr->prompt);

	rc = tinput_read(tinput, &str);
	if (rc == ENOENT) {
		/* User requested exit */
		cli_quit = 1;
		putchar('\n');
		return;
	}

	if (rc != EOK) {
		/* Error in communication with console */
		cli_quit = 1;
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

	tinput_set_compl_ops(tinput, &compl_ops);

	return 0;
}
