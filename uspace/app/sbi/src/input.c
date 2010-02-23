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
 * Reads source code from a file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mytypes.h"
#include "strtab.h"

#include "input.h"

#define INPUT_BUFFER_SIZE 256

static int input_init(input_t *input, char *fname);

/** Create new input object. */
int input_new(input_t **input, char *fname)
{
	*input = malloc(sizeof(input_t));
	if (*input == NULL)
		return ENOMEM;

	return input_init(*input, fname);
}

/** Initialize input object. */
static int input_init(input_t *input, char *fname)
{
	FILE *f;

	f = fopen(fname, "rt");
	if (f == NULL)
		return ENOENT;

	input->buffer = malloc(INPUT_BUFFER_SIZE);
	if (input->buffer == NULL)
		return ENOMEM;

	input->line_no = 0;
	input->fin = f;
	return EOK;
}

/** Get next line of input. */
int input_get_line(input_t *input, char **line)
{
	if (fgets(input->buffer, INPUT_BUFFER_SIZE, input->fin) == NULL)
		input->buffer[0] = '\0';

	if (ferror(input->fin))
		return EIO;

	*line = input->buffer;
	++input->line_no;
	return EOK;
}

/** Get number of the last provided line of input. */
int input_get_line_no(input_t *input)
{
	return input->line_no;
}
