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

/** @file POSIX-specific code. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "../mytypes.h"

#include "os.h"

/*
 * The string functions are in fact standard C, but would not work under
 * HelenOS.
 */

/** Concatenate two strings. */
char *os_str_acat(const char *a, const char *b)
{
	int a_len, b_len;
	char *str;

	a_len = strlen(a);
	b_len = strlen(b);

	str = malloc(a_len + b_len + 1);
	if (str == NULL) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	memcpy(str, a, a_len);
	memcpy(str + a_len, b, b_len);
	str[a_len + b_len] = '\0';

	return str;
}

/** Compare two strings. */
int os_str_cmp(const char *a, const char *b)
{
	return strcmp(a, b);
}

/** Duplicate string. */
char *os_str_dup(const char *str)
{
	return strdup(str);
}

/** Get character from string at the given index. */
int os_str_get_char(const char *str, int index, int *out_char)
{
	size_t len;

	len = strlen(str);
	if (index < 0 || (size_t) index >= len)
		return EINVAL;

	*out_char = str[index];
	return EOK;
}

#define OS_INPUT_BUFFER_SIZE 256
static char os_input_buffer[OS_INPUT_BUFFER_SIZE];

/** Read one line of input from the user. */
int os_input_line(char **ptr)
{
	if (fgets(os_input_buffer, OS_INPUT_BUFFER_SIZE, stdin) == NULL)
		os_input_buffer[0] = '\0';

	if (ferror(stdin)) {
		*ptr = NULL;
		return EIO;
	}

	*ptr = strdup(os_input_buffer);
	return EOK;
}

/** Simple command execution. */
int os_exec(char *const cmd[])
{
	pid_t pid;
	int status;

	pid = vfork();

	if (pid == 0) {
		execvp(cmd[0], cmd);
		/* If we get here, exec failed. */
		exit(1);
	} else if (pid == -1) {
		/* fork() failed */
		return EBUSY;
	}

	if (waitpid(pid, &status, 0) == -1) {
		/* waitpid() failed */
		printf("Error: Waitpid failed.\n");
		exit(1);
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		printf("Error: Exec failed or child returned non-zero "
		    "exit status.\n");
	}

	return EOK;
}
