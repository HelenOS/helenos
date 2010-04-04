/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @file HelenOS-specific code. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <tinput.h>

#include "os.h"

/*
 * Using HelenOS-specific string API.
 */

static tinput_t *tinput = NULL;

/** Concatenate two strings. */
char *os_str_acat(const char *a, const char *b)
{
	int a_size, b_size;
	char *str;

	a_size = str_size(a);
	b_size = str_size(b);

	str = malloc(a_size + b_size + 1);
	if (str == NULL) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	memcpy(str, a, a_size);
	memcpy(str + a_size, b, b_size);
	str[a_size + b_size] = '\0';

	return str;
}

/** Compare two strings. */
int os_str_cmp(const char *a, const char *b)
{
	return str_cmp(a, b);
}

/** Duplicate string. */
char *os_str_dup(const char *str)
{
	return str_dup(str);
}

/** Get character from string at the given index. */
int os_str_get_char(const char *str, int index, int *out_char)
{
	size_t offset;
	int i;
	wchar_t c;

	if (index < 0)
		return EINVAL;

	offset = 0;
	for (i = 0; i <= index; ++i) {
		c = str_decode(str, &offset, STR_NO_LIMIT);
		if (c == '\0')
			return EINVAL;
		if (c == U_SPECIAL)
			return EIO;
	}

	*out_char = (int) c;
	return EOK;
}

/** Read one line of input from the user. */
int os_input_line(char **ptr)
{
	char *line;
	int rc;

	if (tinput == NULL) {
		tinput = tinput_new();
		if (tinput == NULL)
			return EIO;
	}

	rc = tinput_read(tinput, &line);
	if (rc == ENOENT) {
		/* User-requested abort */
		*ptr = os_str_dup("");
		return EOK;
	}

	if (rc != EOK) {
		/* Error in communication with console */
		return EIO;
	}

	/* XXX Input module needs trailing newline to keep going. */
	*ptr = os_str_acat(line, "\n");
	free(line);

	return EOK;
}

/** Simple command execution. */
int os_exec(char *const cmd[])
{
	task_id_t tid;
	task_exit_t texit;
	int retval;

	tid = task_spawn(cmd[0], (char const * const *) cmd);
	if (tid == 0) {
		printf("Error: Failed spawning '%s'.\n", cmd[0]);
		exit(1);
	}

	/* XXX Handle exit status and return value. */
	task_wait(tid, &texit, &retval);

	return EOK;
}
