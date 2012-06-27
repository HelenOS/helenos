/*
 * Copyright (c) 2012 Sean Bartell
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

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Script parsing.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include "script.h"
#include "transform.h"
#include "tree.h"

/** Tokens with more characters than this may be read incorrectly. */
#define MAX_TOKEN_SIZE 256
#define BUFFER_SIZE 4096

/** Single-character symbols are represented by the character itself. Every
 * other token uses one of these values: */
typedef enum {
	TOKEN_ERROR = -128,
	TOKEN_EOF,
	TOKEN_IDENTIFIER,
	TOKEN_LEFT_ARROW,

	/* Keywords */
	TOKEN_STRUCT,
	TOKEN_TRANSFORM,
} token_type_t;

/** Singly-linked list of named transforms. */
typedef struct transform_list {
	char *name;
	bithenge_transform_t *transform;
	struct transform_list *next;
} transform_list_t;

/** State kept by the parser. */
typedef struct {
	/** Rather than constantly checking return values, the parser uses this
	 * to indicate whether an error has occurred. */
	int error;
	/** The list of named transforms. */
	transform_list_t *transform_list;
	/** The name of the script file. */
	const char *filename;
	/** The script file being read from. */
	FILE *file;
	/** The buffer that holds script code. There is always a '\0' after the
	 * current position to prevent reading too far. */
	char buffer[BUFFER_SIZE];
	/** The start position within the buffer of the next unread token. */
	size_t buffer_pos;
	/** The start position within the buffer of the current token. */
	size_t old_buffer_pos;
	/** The line number of the current token. */
	int lineno;
	/** Added to a buffer position to find the column number. */
	int line_offset;
	/** The type of the current token. */
	token_type_t token;
	union {
		/** The value of a TOKEN_IDENTIFIER token. Unless changed to
		 * NULL, it will be freed when the next token is read. */
		char *token_string;
	};
} state_t;

/** Free the previous token's data. This must be called before changing
 * state->token. */
static void done_with_token(state_t *state)
{
	if (state->token == TOKEN_IDENTIFIER)
		free(state->token_string);
	state->token = TOKEN_ERROR;
}

/** Note that an error has occurred. */
static void error_errno(state_t *state, int error)
{
	// Don't overwrite a previous error.
	if (state->error == EOK) {
		done_with_token(state);
		state->token = TOKEN_ERROR;
		state->error = error;
	}
}

/** Note that a syntax error has occurred and print an error message. */
static void syntax_error(state_t *state, const char *message)
{
	// Printing multiple errors is confusing.
	if (state->error == EOK) {
		size_t start_char = state->old_buffer_pos + state->line_offset;
		size_t end_char = state->buffer_pos + state->line_offset;
		size_t size = end_char - start_char;
		fprintf(stderr, "%s:%d:", state->filename, state->lineno);
		if (size <= 1)
			fprintf(stderr, "%zd: ", start_char);
		else
			fprintf(stderr, "%zd-%zd: ", start_char, end_char - 1);
		fprintf(stderr, "%s: \"%.*s\"\n", message, (int)size,
		    state->buffer + state->old_buffer_pos);
		error_errno(state, EINVAL);
	}
}

/** Ensure the buffer contains enough characters to read a token. */
static void fill_buffer(state_t *state)
{
	if (state->buffer_pos + MAX_TOKEN_SIZE < BUFFER_SIZE)
		return;

	size_t empty_size = state->buffer_pos;
	size_t used_size = BUFFER_SIZE - 1 - state->buffer_pos;
	memmove(state->buffer, state->buffer + state->buffer_pos, used_size);
	state->line_offset += state->buffer_pos;
	state->buffer_pos = 0;

	size_t read_size = fread(state->buffer + used_size, 1, empty_size,
	    state->file);
	if (ferror(state->file))
		error_errno(state, EIO);
	state->buffer[used_size + read_size] = '\0';
}

/** Read the next token. */
static void next_token(state_t *state)
{
	fill_buffer(state);
	done_with_token(state);
	state->old_buffer_pos = state->buffer_pos;
	char ch = state->buffer[state->buffer_pos];
	if (ch == '\0') {
		state->token = TOKEN_EOF;
	} else if (isspace(ch)) {
		// Will eventually reach the '\0' at the end
		while (isspace(state->buffer[state->buffer_pos])) {
			if (state->buffer[state->buffer_pos] == '\n') {
				state->lineno++;
				state->line_offset = -state->buffer_pos;
			}
			state->buffer_pos++;
		}
		next_token(state);
		return;
	} else if (isalpha(ch)) {
		while (isalnum(state->buffer[state->buffer_pos]))
			state->buffer_pos++;
		char *value = str_ndup(state->buffer + state->old_buffer_pos,
		    state->buffer_pos - state->old_buffer_pos);
		if (!value) {
			error_errno(state, ENOMEM);
		} else if (!str_cmp(value, "struct")) {
			state->token = TOKEN_STRUCT;
			free(value);
		} else if (!str_cmp(value, "transform")) {
			state->token = TOKEN_TRANSFORM;
			free(value);
		} else {
			state->token = TOKEN_IDENTIFIER;
			state->token_string = value;
		}
	} else if (ch == '<') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '-') {
			state->buffer_pos++;
			state->token = TOKEN_LEFT_ARROW;
		}
	} else {
		state->token = ch;
		state->buffer_pos++;
	}
}

/** Allocate memory and handle failure by setting the error in the state. The
 * caller must check the state for errors before using the return value of this
 * function. */
static void *state_malloc(state_t *state, size_t size)
{
	if (state->error != EOK)
		return NULL;
	void *result = malloc(size);
	if (result == NULL)
		error_errno(state, ENOMEM);
	return result;
}

/** Reallocate memory and handle failure by setting the error in the state. If
 * an error occurs, the existing pointer will be returned. */
static void *state_realloc(state_t *state, void *ptr, size_t size)
{
	if (state->error != EOK)
		return ptr;
	void *result = realloc(ptr, size);
	if (result == NULL) {
		error_errno(state, ENOMEM);
		return ptr;
	}
	return result;
}

/** Expect and consume a certain token. If the next token is of the wrong type,
 * an error is caused. */
static void expect(state_t *state, token_type_t type)
{
	if (state->token != type) {
		syntax_error(state, "unexpected");
		return;
	}
	next_token(state);
}

/** Expect and consume an identifier token. If the next token is not an
 * identifier, an error is caused and this function returns null. */
static char *expect_identifier(state_t *state)
{
	if (state->token != TOKEN_IDENTIFIER) {
		syntax_error(state, "unexpected (identifier expected)");
		return NULL;
	}
	char *val = state->token_string;
	state->token_string = NULL;
	next_token(state);
	return val;
}

/** Find a transform by name. A reference will be added to the transform.
 * @return The found transform, or NULL if none was found. */
static bithenge_transform_t *get_named_transform(state_t *state,
    const char *name)
{
	for (transform_list_t *e = state->transform_list; e; e = e->next) {
		if (!str_cmp(e->name, name)) {
			bithenge_transform_inc_ref(e->transform);
			return e->transform;
		}
	}
	for (int i = 0; bithenge_primitive_transforms[i].name; i++) {
		if (!str_cmp(bithenge_primitive_transforms[i].name, name)) {
			bithenge_transform_t *xform =
			    bithenge_primitive_transforms[i].transform;
			bithenge_transform_inc_ref(xform);
			return xform;
		}
	}
	return NULL;
}

/** Add a named transform. This function takes ownership of the name and a
 * reference to the transform. If an error has occurred, either may be null. */
static void add_named_transform(state_t *state, bithenge_transform_t *xform, char *name)
{
	transform_list_t *entry = state_malloc(state, sizeof(*entry));
	if (state->error != EOK) {
		free(name);
		bithenge_transform_dec_ref(xform);
		free(entry);
		return;
	}
	entry->name = name;
	entry->transform = xform;
	entry->next = state->transform_list;
	state->transform_list = entry;
}

static bithenge_transform_t *parse_transform(state_t *state);

static bithenge_transform_t *parse_struct(state_t *state)
{
	size_t num = 0;
	bithenge_named_transform_t *subxforms;
	/* We keep an extra space for the {NULL, NULL} terminator. */
	subxforms = state_malloc(state, sizeof(*subxforms));
	expect(state, TOKEN_STRUCT);
	expect(state, '{');
	while (state->error == EOK && state->token != '}') {
		expect(state, '.');
		subxforms[num].name = expect_identifier(state);
		expect(state, TOKEN_LEFT_ARROW);
		subxforms[num].transform = parse_transform(state);
		expect(state, ';');
		num++;
		subxforms = state_realloc(state, subxforms,
		    (num + 1)*sizeof(*subxforms));
	}
	expect(state, '}');

	if (state->error != EOK) {
		while (num--) {
			free((void *)subxforms[num].name);
			bithenge_transform_dec_ref(subxforms[num].transform);
		}
		free(subxforms);
		return NULL;
	}

	subxforms[num].name = NULL;
	subxforms[num].transform = NULL;
	bithenge_transform_t *result;
	int rc = bithenge_new_struct(&result, subxforms);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}
	return result;
}

/** Parse a transform. 
 * @return The parsed transform, or NULL if an error occurred. */
static bithenge_transform_t *parse_transform(state_t *state)
{
	if (state->token == TOKEN_IDENTIFIER) {
		bithenge_transform_t *result = get_named_transform(state,
		    state->token_string);
		if (!result)
			syntax_error(state, "transform not found");
		next_token(state);
		return result;
	} else if (state->token == TOKEN_STRUCT) {
		return parse_struct(state);
	} else {
		syntax_error(state, "unexpected (transform expected)");
		return NULL;
	}
}

/** Parse a definition. */
static void parse_definition(state_t *state)
{
	expect(state, TOKEN_TRANSFORM);
	char *name = expect_identifier(state);
	expect(state, '=');
	bithenge_transform_t *xform = parse_transform(state);
	expect(state, ';');
	add_named_transform(state, xform, name);
}

/** Initialize the state. */
static void state_init(state_t *state, const char *filename)
{
	state->error = EOK;
	state->transform_list = NULL;
	state->token = TOKEN_ERROR;
	state->old_buffer_pos = state->buffer_pos = BUFFER_SIZE - 1;
	state->lineno = 1;
	state->line_offset = (int)-state->buffer_pos + 1;
	state->filename = filename;
	state->file = fopen(filename, "r");
	if (!state->file) {
		error_errno(state, errno);
	} else {
		next_token(state);
	}
}

/** Destroy the state. */
static void state_destroy(state_t *state)
{
	done_with_token(state);
	state->token = TOKEN_ERROR;
	fclose(state->file);
	transform_list_t *entry = state->transform_list;
	while (entry) {
		transform_list_t *next = entry->next;
		free(entry->name);
		bithenge_transform_dec_ref(entry->transform);
		free(entry);
		entry = next;
	}
}

/** Parse a script file.
 * @param filename The name of the script file.
 * @param[out] out Stores the "main" transform.
 * @return EOK on success, EINVAL on syntax error, or an error code from
 * errno.h. */
int bithenge_parse_script(const char *filename, bithenge_transform_t **out)
{
	state_t state;
	state_init(&state, filename);
	while (state.error == EOK && state.token != TOKEN_EOF)
		parse_definition(&state);
	*out = get_named_transform(&state, "main");
	int rc = state.error;
	state_destroy(&state);
	if (rc == EOK && !*out) {
		fprintf(stderr, "no \"main\" transform\n");
		rc = EINVAL;
	}
	return rc;
}

/** @}
 */
