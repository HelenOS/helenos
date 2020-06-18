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

#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types/common.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "../mytypes.h"

#include "os.h"

/** Path to executable file via which we have been invoked. */
static char *ef_path;

/*
 * The string functions are in fact standard C, but would not work under
 * HelenOS.
 *
 * XXX String functions used here only work with 8-bit text encoding.
 */

/** Concatenate two strings.
 *
 * @param a	First string
 * @param b	Second string
 * @return	New string, concatenation of @a a and @a b.
 */
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

/** Return slice (substring) of a string.
 *
 * Copies the specified range of characters from @a str and returns it
 * as a newly allocated string. @a start + @a length must be less than
 * or equal to the length of @a str.
 *
 * @param str		String
 * @param start		Index of first character (starting from zero).
 * @param length	Number of characters to copy.
 *
 * @return		Newly allocated string holding the slice.
 */
char *os_str_aslice(const char *str, size_t start, size_t length)
{
	char *slice;

	assert(start + length <= strlen(str));
	slice = malloc(length + 1);
	if (slice == NULL) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	strncpy(slice, str + start, length);
	slice[length] = '\0';

	return slice;
}

/** Compare two strings.
 *
 * @param a	First string
 * @param b	Second string
 * @return	Zero if equal, nonzero if not equal
 */
errno_t os_str_cmp(const char *a, const char *b)
{
	return strcmp(a, b);
}

/** Return number of characters in string.
 *
 * @param str	String
 * @return	Number of characters in @a str.
 */
size_t os_str_length(const char *str)
{
	return strlen(str);
}

/** Duplicate string.
 *
 * @param str	String
 * @return	New string, duplicate of @a str.
 */
char *os_str_dup(const char *str)
{
	return strdup(str);
}

/** Get character from string at the given index.
 *
 * @param str		String
 * @param index		Character index (starting from zero).
 * @param out_char	Place to store character.
 * @return		EOK on success, EINVAL if index is out of bounds,
 *			EIO on decoding error.
 */
errno_t os_str_get_char(const char *str, int index, int *out_char)
{
	size_t len;

	len = strlen(str);
	if (index < 0 || (size_t) index >= len)
		return EINVAL;

	*out_char = str[index];
	return EOK;
}

/** Convert character to new string.
 *
 * @param chr		Character
 * @return		Newly allocated string.
 */
char *os_chr_to_astr(char32_t chr)
{
	char *str;

	str = malloc(2);
	if (str == NULL) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	str[0] = (char) chr;
	str[1] = '\0';

	return str;
}

#define OS_INPUT_BUFFER_SIZE 256
static char os_input_buffer[OS_INPUT_BUFFER_SIZE];

/** Display survival help message. */
void os_input_disp_help(void)
{
	printf("Send ^C (SIGINT) to quit.\n");
}

/** Read one line of input from the user.
 *
 * @param ptr	Place to store pointer to new string.
 */
errno_t os_input_line(const char *prompt, char **ptr)
{
	printf("%s", prompt);

	if (fgets(os_input_buffer, OS_INPUT_BUFFER_SIZE, stdin) == NULL)
		os_input_buffer[0] = '\0';

	if (ferror(stdin)) {
		*ptr = NULL;
		return EIO;
	}

	*ptr = strdup(os_input_buffer);
	return EOK;
}

/** Simple command execution.
 *
 * @param cmd	Command and arguments (NULL-terminated list of strings.)
 *		Command is present just one, not duplicated.
 */
errno_t os_exec(char *const cmd[])
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

/** Store the executable file path via which we were executed.
 *
 * @param path	Executable path via which we were executed.
 */
void os_store_ef_path(char *path)
{
	ef_path = path;
}

/** Return path to the Sysel library
 *
 * @return New string. Caller should deallocate it using @c free().
 */
char *os_get_lib_path(void)
{
	return os_str_acat(dirname(ef_path), "/lib");
}
