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

/** @file Input module
 *
 * Reads source code. Currently input can be read from a file (standard
 * case), from a string literal (when parsing builtin code) or interactively
 * from the user.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mytypes.h"
#include "os/os.h"
#include "strtab.h"

#include "input.h"

/** Size of input buffer. XXX This limits the maximum line length. */
#define INPUT_BUFFER_SIZE 256

static errno_t input_init_file(input_t *input, const char *fname);
static void input_init_interactive(input_t *input);
static void input_init_string(input_t *input, const char *str);

/** Create new input object for reading from file.
 *
 * @param input		Place to store pointer to new input object.
 * @param fname		Name of file to read from.
 *
 * @return		EOK on success, ENOMEM when allocation fails,
 *			ENOENT when opening file fails.
 */
errno_t input_new_file(input_t **input, const char *fname)
{
	*input = malloc(sizeof(input_t));
	if (*input == NULL)
		return ENOMEM;

	return input_init_file(*input, fname);
}

/** Create new input object for reading from interactive input.
 *
 * @param input		Place to store pointer to new input object.
 * @return		EOK on success, ENOMEM when allocation fails.
 */
errno_t input_new_interactive(input_t **input)
{
	*input = malloc(sizeof(input_t));
	if (*input == NULL)
		return ENOMEM;

	input_init_interactive(*input);
	return EOK;
}

/** Create new input object for reading from string.
 *
 * @param input		Place to store pointer to new input object.
 * @param str		String literal from which to read input.
 * @return		EOK on success, ENOMEM when allocation fails.
 */
errno_t input_new_string(input_t **input, const char *str)
{
	*input = malloc(sizeof(input_t));
	if (*input == NULL)
		return ENOMEM;

	input_init_string(*input, str);
	return EOK;
}

/** Initialize input object for reading from file.
 *
 * @param input		Input object.
 * @param fname		Name of file to read from.
 *
 * @return		EOK on success, ENOENT when opening file fails.
 */
static errno_t input_init_file(input_t *input, const char *fname)
{
	FILE *f;

	f = fopen(fname, "rt");
	if (f == NULL)
		return ENOENT;

	input->buffer = malloc(INPUT_BUFFER_SIZE);
	if (input->buffer == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	input->name = os_str_dup(fname);
	input->str = NULL;
	input->line_no = 0;
	input->fin = f;
	return EOK;
}

/** Initialize input object for reading from interactive input.
 *
 * @param input		Input object.
 */
static void input_init_interactive(input_t *input)
{
	input->buffer = malloc(INPUT_BUFFER_SIZE);
	if (input->buffer == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	input->name = "<user-input>";
	input->str = NULL;
	input->line_no = 0;
	input->fin = NULL;
}

/** Initialize input object for reading from string.
 *
 * @param input		Input object.
 * @param str		String literal from which to read input.
 */
static void input_init_string(input_t *input, const char *str)
{
	input->buffer = malloc(INPUT_BUFFER_SIZE);
	if (input->buffer == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	input->name = "<builtin>";
	input->str = str;
	input->line_no = 0;
	input->fin = NULL;
}

/** Get next line of input.
 *
 * The pointer stored in @a line is owned by @a input and is valid until the
 * next call to input_get_line(). The caller is not to free it. The returned
 * line is terminated with '\n' if another line is coming (potentially empty).
 * An empty line ("") signals end of input.
 *
 * @param input		Input object.
 * @param line		Place to store pointer to next line.
 *
 * @return		EOK on success, EIO on failure.
 */
errno_t input_get_line(input_t *input, char **line)
{
	const char *prompt;
	const char *sp;
	char *dp;
	char *line_p;
	size_t cnt;

	if (input->fin != NULL) {
		/* Reading from file. */
		if (fgets(input->buffer, INPUT_BUFFER_SIZE, input->fin) == NULL)
			input->buffer[0] = '\0';

		if (ferror(input->fin))
			return EIO;

		*line = input->buffer;
	} else if (input->str != NULL) {
		/* Reading from a string constant. */

		/* Copy one line. */
		sp = input->str;
		dp = input->buffer;
		cnt = 0;
		while (*sp != '\n' && *sp != '\0' &&
		    cnt < INPUT_BUFFER_SIZE - 2) {
			*dp++ = *sp++;
		}

		/* Advance to start of next line. */
		if (*sp == '\n')
			*dp++ = *sp++;

		*dp = '\0';
		input->str = sp;
		*line = input->buffer;
	} else {
		/* Interactive mode */
		if (input->line_no == 0)
			prompt = "sbi> ";
		else
			prompt = "...  ";

		fflush(stdout);
		if (os_input_line(prompt, &line_p) != EOK)
			return EIO;

		*line = line_p;
	}

	++input->line_no;
	return EOK;
}

/** Get number of the last provided line of input.
 *
 * @param input		Input object.
 * @return		Line number of the last provided input line (counting
 *			from 1 up).
 */
int input_get_line_no(input_t *input)
{
	return input->line_no;
}
