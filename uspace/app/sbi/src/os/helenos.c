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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <tinput.h>
#include <str_error.h>

#include "os.h"

/** Path to executable file via which we have been invoked. */
static char *ef_path;

/*
 * Using HelenOS-specific string API.
 */

static tinput_t *tinput = NULL;

/** Concatenate two strings.
 *
 * @param a	First string
 * @param b	Second string
 * @return	New string, concatenation of @a a and @a b.
 */
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
	size_t offset;
	size_t i;
	size_t size;
	wchar_t c;

	assert(start + length <= str_length(str));

	offset = 0;
	for (i = 0; i < start; ++i) {
		c = str_decode(str, &offset, STR_NO_LIMIT);
		assert(c != '\0');
		assert(c != U_SPECIAL);
		(void) c;
	}

	size = str_lsize(str, length);
	slice = str_ndup(str + offset, size);

	return slice;
}

/** Compare two strings.
 *
 * @param a	First string
 * @param b	Second string
 * @return	Zero if equal, nonzero if not equal
 */
int os_str_cmp(const char *a, const char *b)
{
	return str_cmp(a, b);
}

/** Return number of characters in string.
 *
 * @param str	String
 * @return	Number of characters in @a str.
 */
size_t os_str_length(const char *str)
{
	return str_length(str);
}

/** Duplicate string.
 *
 * @param str	String
 * @return	New string, duplicate of @a str.
 */
char *os_str_dup(const char *str)
{
	return str_dup(str);
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
	size_t offset;
	int i;
	wchar_t c = 0;

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

/** Convert character to new string.
 *
 * @param chr		Character
 * @return		Newly allocated string.
 */
char *os_chr_to_astr(wchar_t chr)
{
	char *str;
	size_t offset;

	str = malloc(STR_BOUNDS(1) + 1);
	if (str == NULL) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	offset = 0;
	if (chr_encode(chr, str, &offset, STR_BOUNDS(1)) != EOK) {
		/* XXX Should handle gracefully */
		printf("String conversion error.\n");
		exit(1);
	}

	str[offset] = '\0';
	return str;
}

/** Display survival help message. */
void os_input_disp_help(void)
{
	printf("Press Ctrl-Q to quit.\n");
}

/** Read one line of input from the user.
 *
 * @param ptr	Place to store pointer to new string.
 */
errno_t os_input_line(const char *prompt, char **ptr)
{
	char *line;
	errno_t rc;

	if (tinput == NULL) {
		tinput = tinput_new();
		if (tinput == NULL)
			return EIO;

		tinput_set_prompt(tinput, prompt);
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

/** Simple command execution.
 *
 * @param cmd	Command and arguments (NULL-terminated list of strings.)
 *		Command is present just one, not duplicated.
 */
errno_t os_exec(char *const cmd[])
{
	task_id_t tid;
	task_wait_t twait;
	task_exit_t texit;
	errno_t rc;
	int retval;

	rc = task_spawnv(&tid, &twait, cmd[0], (char const * const *) cmd);
	if (rc != EOK) {
		printf("Error: Failed spawning '%s' (%s).\n", cmd[0],
		    str_error(rc));
		exit(1);
	}

	/* XXX Handle exit status and return value. */
	rc = task_wait(&twait, &texit, &retval);
	(void) rc;

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
	return os_str_dup("/src/sysel/lib");
}
