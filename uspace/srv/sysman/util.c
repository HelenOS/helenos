/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include "util.h"

/** Compose path to file inside a directory.
 *
 * @return	pointer to newly allocated string
 * @return	NULL on error
 */
char *util_compose_path(const char *dirname, const char *filename)
{
	size_t size = str_size(dirname) + str_size(filename) + 2;
	char *result = malloc(sizeof(char) * size);

	if (result == NULL) {
		return NULL;
	}
	if (snprintf(result, size, "%s/%s", dirname, filename) < 0) {
		return NULL;
	}
	return result;
}

/** Parse command line
 *
 * @param[out]  dst      pointer to command_t
 *                       path and zeroth argument are the equal
 *
 * @return  true   on success
 * @return  false  on (low memory) error
 */
bool util_parse_command(const char *string, void *dst, text_parse_t *parse,
    size_t lineno)
{
	command_t *command = dst;
	util_command_deinit(command);

	command->buffer = str_dup(string);
	if (!command->buffer) {
		return false;
	}

	command->argc = 0;
	char *to_split = command->buffer;
	char *cur_tok;
	bool has_path = false;

	while ((cur_tok = str_tok(to_split, " ", &to_split)) &&
	    command->argc < MAX_COMMAND_ARGS) {
		if (!has_path) {
			command->path = cur_tok;
			has_path = true;
		}
		command->argv[command->argc++] = cur_tok;
	}
	command->argv[command->argc] = NULL;

	if (command->argc > MAX_COMMAND_ARGS) {
		text_parse_raise_error(parse, lineno,
		    CONFIGURATION_ELIMIT);
		return false;
	} else {
		return true;
	}
}

void util_command_init(command_t *command)
{
	memset(command, 0, sizeof(*command));
}

void util_command_deinit(command_t *command)
{
	free(command->buffer);
	memset(command, 0, sizeof(*command));
}
