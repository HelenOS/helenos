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
#include <console.h>
#include <kbd/kbd.h>
#include <kbd/keycode.h>
#include <errno.h>
#include <bool.h>

#include "config.h"
#include "util.h"
#include "scli.h"
#include "input.h"
#include "errors.h"
#include "exec.h"

static void read_line(char *, int);

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

	tmp = strdup(usr->line);

	cmd[n] = strtok(tmp, " ");
	while (cmd[n] && n < WORD_MAX) {
		cmd[++n] = strtok(NULL, " ");
	}

	/* We have rubbish */
	if (NULL == cmd[0]) {
		rc = CL_ENOENT;
		goto finit;
	}

	/* Its a builtin command ? */
	if ((i = (is_builtin(cmd[0]))) > -1) {
		rc = run_builtin(i, cmd, usr);
		goto finit;
	/* Its a module ? */
	} else if ((i = (is_module(cmd[0]))) > -1) {
		rc = run_module(i, cmd);
		goto finit;
	}

	/* See what try_exec thinks of it */
	rc = try_exec(cmd[0], cmd);

finit:
	if (NULL != usr->line) {
		free(usr->line);
		usr->line = (char *) NULL;
	}
	if (NULL != tmp)
		free(tmp);

	return rc;
}

static void read_line(char *buffer, int n)
{
	kbd_event_t ev;
	size_t offs, otmp;
	wchar_t dec;

	offs = 0;
	while (true) {
		fflush(stdout);
		if (kbd_get_event(&ev) < 0)
			return;
		if (ev.type == KE_RELEASE)
			continue;

		if (ev.key == KC_ENTER || ev.key == KC_NENTER)
			break;
		if (ev.key == KC_BACKSPACE) {
			if (offs > 0) {
				/*
				 * Back up until we reach valid start of
				 * character.
				 */
				while (offs > 0) {
					--offs; otmp = offs;
					dec = str_decode(buffer, &otmp, n);
					if (dec != U_SPECIAL)
						break;
				}
				putchar('\b');
			}
			continue;
		}
		if (ev.c >= ' ') {
			//putchar(ev.c);
			if (chr_encode(ev.c, buffer, &offs, n - 1) == EOK)
				console_putchar(ev.c);
		}
	}
	putchar('\n');
	buffer[offs] = '\0';
}

/* TODO:
 * Implement something like editline() / readline(), if even
 * just for command history and making arrows work. */
void get_input(cliuser_t *usr)
{
	char line[INPUT_MAX];

	console_set_style(STYLE_EMPHASIS);
	printf("%s", usr->prompt);
	console_set_style(STYLE_NORMAL);

	read_line(line, INPUT_MAX);
	/* Make sure we don't have rubbish or a C/R happy user */
	if (str_cmp(line, "") == 0 || str_cmp(line, "\n") == 0)
		return;
	usr->line = strdup(line);

	return;
}

