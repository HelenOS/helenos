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
#include <bithenge/compound.h>
#include <bithenge/expression.h>
#include <bithenge/script.h>
#include <bithenge/sequence.h>
#include <bithenge/transform.h>
#include <bithenge/tree.h>
#include "common.h"

/** @cond internal */
#define BUFFER_SIZE 4096
/** @endcond */

/** Tokens with more characters than this may be read incorrectly. */
static const int MAX_TOKEN_SIZE = 256;

/** @cond internal
 * Single-character symbols are represented by the character itself. Every
 * other token uses one of these values: */
typedef enum {
	TOKEN_ERROR = -128,

	TOKEN_AND,
	TOKEN_CONCAT,
	TOKEN_EQUALS,
	TOKEN_EOF,
	TOKEN_GREATER_THAN_OR_EQUAL,
	TOKEN_IDENTIFIER,
	TOKEN_INTEGER,
	TOKEN_INTEGER_DIVIDE,
	TOKEN_LEFT_ARROW,
	TOKEN_LESS_THAN_OR_EQUAL,
	TOKEN_NOT_EQUAL,
	TOKEN_OR,

	/* Keywords */
	TOKEN_DO,
	TOKEN_ELSE,
	TOKEN_FALSE,
	TOKEN_IF,
	TOKEN_IN,
	TOKEN_PARTIAL,
	TOKEN_REPEAT,
	TOKEN_STRUCT,
	TOKEN_SWITCH,
	TOKEN_TRANSFORM,
	TOKEN_TRUE,
	TOKEN_WHILE,
} token_type_t;
/** @endcond */

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
	errno_t error;

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
		/** The value of a TOKEN_INTEGER token. */
		bithenge_int_t token_int;
	};

	/** The names of the current transform's parameters. */
	char **parameter_names;
	/** The number of parameters. */
	int num_params;
	/** @a parse_expression sets this when TOKEN_IN is used. */
	bool in_node_used;
} state_t;

/** Free the previous token's data. This must be called before changing
 * state->token. */
static void done_with_token(state_t *state)
{
	if (state->token == TOKEN_IDENTIFIER)
		free(state->token_string);
	state->token = TOKEN_ERROR;
}

/** Note that an error has occurred if error is not EOK. */
static void error_errno(state_t *state, errno_t error)
{
	// Don't overwrite a previous error.
	if (state->error == EOK && error != EOK) {
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
	} else if (ch == '#') {
		while (state->buffer[state->buffer_pos] != '\n'
		    && state->buffer[state->buffer_pos] != '\0') {
			state->buffer_pos++;
			fill_buffer(state);
		}
		next_token(state);
		return;
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
		while (isalnum(state->buffer[state->buffer_pos])
		    || state->buffer[state->buffer_pos] == '_')
			state->buffer_pos++;
		char *value = str_ndup(state->buffer + state->old_buffer_pos,
		    state->buffer_pos - state->old_buffer_pos);
		if (!value) {
			error_errno(state, ENOMEM);
		} else if (!str_cmp(value, "do")) {
			state->token = TOKEN_DO;
			free(value);
		} else if (!str_cmp(value, "else")) {
			state->token = TOKEN_ELSE;
			free(value);
		} else if (!str_cmp(value, "false")) {
			state->token = TOKEN_FALSE;
			free(value);
		} else if (!str_cmp(value, "if")) {
			state->token = TOKEN_IF;
			free(value);
		} else if (!str_cmp(value, "in")) {
			state->token = TOKEN_IN;
			free(value);
		} else if (!str_cmp(value, "partial")) {
			state->token = TOKEN_PARTIAL;
			free(value);
		} else if (!str_cmp(value, "repeat")) {
			state->token = TOKEN_REPEAT;
			free(value);
		} else if (!str_cmp(value, "struct")) {
			state->token = TOKEN_STRUCT;
			free(value);
		} else if (!str_cmp(value, "switch")) {
			state->token = TOKEN_SWITCH;
			free(value);
		} else if (!str_cmp(value, "transform")) {
			state->token = TOKEN_TRANSFORM;
			free(value);
		} else if (!str_cmp(value, "true")) {
			state->token = TOKEN_TRUE;
			free(value);
		} else if (!str_cmp(value, "while")) {
			state->token = TOKEN_WHILE;
			free(value);
		} else {
			state->token = TOKEN_IDENTIFIER;
			state->token_string = value;
		}
	} else if (isdigit(ch)) {
		while (isdigit(state->buffer[state->buffer_pos]))
			state->buffer_pos++;
		state->token = TOKEN_INTEGER;
		errno_t rc = bithenge_parse_int(state->buffer +
		    state->old_buffer_pos, &state->token_int);
		error_errno(state, rc);
	} else if (ch == '<') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '-') {
			state->buffer_pos++;
			state->token = TOKEN_LEFT_ARROW;
		} else if (state->buffer[state->buffer_pos] == '=') {
			state->buffer_pos++;
			state->token = TOKEN_LESS_THAN_OR_EQUAL;
		}
	} else if (ch == '>') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '=') {
			state->buffer_pos++;
			state->token = TOKEN_GREATER_THAN_OR_EQUAL;
		}
	} else if (ch == '=') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '=') {
			state->token = TOKEN_EQUALS;
			state->buffer_pos++;
		}
	} else if (ch == '/') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '/') {
			state->token = TOKEN_INTEGER_DIVIDE;
			state->buffer_pos++;
		}
	} else if (ch == '!') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '=') {
			state->token = TOKEN_NOT_EQUAL;
			state->buffer_pos++;
		}
	} else if (ch == '&') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '&') {
			state->token = TOKEN_AND;
			state->buffer_pos++;
		}
	} else if (ch == '|') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '|') {
			state->token = TOKEN_OR;
			state->buffer_pos++;
		}
	} else if (ch == '+') {
		state->token = ch;
		state->buffer_pos++;
		if (state->buffer[state->buffer_pos] == '+') {
			state->token = TOKEN_CONCAT;
			state->buffer_pos++;
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
static bithenge_transform_t *parse_struct(state_t *state);
static bithenge_expression_t *parse_expression(state_t *state);



/***************** Expressions                               *****************/

/** @cond internal */
typedef enum {
	PRECEDENCE_NONE,
	PRECEDENCE_AND,
	PRECEDENCE_EQUALS,
	PRECEDENCE_COMPARE,
	PRECEDENCE_ADD,
	PRECEDENCE_MULTIPLY,
} precedence_t;
/** @endcond */

static bithenge_binary_op_t token_as_binary_operator(token_type_t token)
{
	switch ((int)token) {
	case '+':
		return BITHENGE_EXPRESSION_ADD;
	case '-':
		return BITHENGE_EXPRESSION_SUBTRACT;
	case '*':
		return BITHENGE_EXPRESSION_MULTIPLY;
	case TOKEN_INTEGER_DIVIDE:
		return BITHENGE_EXPRESSION_INTEGER_DIVIDE;
	case '%':
		return BITHENGE_EXPRESSION_MODULO;
	case '<':
		return BITHENGE_EXPRESSION_LESS_THAN;
	case TOKEN_LESS_THAN_OR_EQUAL:
		return BITHENGE_EXPRESSION_LESS_THAN_OR_EQUAL;
	case '>':
		return BITHENGE_EXPRESSION_GREATER_THAN;
	case TOKEN_GREATER_THAN_OR_EQUAL:
		return BITHENGE_EXPRESSION_GREATER_THAN_OR_EQUAL;
	case TOKEN_EQUALS:
		return BITHENGE_EXPRESSION_EQUALS;
	case TOKEN_NOT_EQUAL:
		return BITHENGE_EXPRESSION_NOT_EQUALS;
	case TOKEN_AND:
		return BITHENGE_EXPRESSION_AND;
	case TOKEN_OR:
		return BITHENGE_EXPRESSION_OR;
	case TOKEN_CONCAT:
		return BITHENGE_EXPRESSION_CONCAT;
	default:
		return BITHENGE_EXPRESSION_INVALID_BINARY_OP;
	}
}

static precedence_t binary_operator_precedence(bithenge_binary_op_t op)
{
	switch (op) {
	case BITHENGE_EXPRESSION_ADD: /* fallthrough */
	case BITHENGE_EXPRESSION_SUBTRACT: /* fallthrough */
	case BITHENGE_EXPRESSION_CONCAT:
		return PRECEDENCE_ADD;
	case BITHENGE_EXPRESSION_MULTIPLY: /* fallthrough */
	case BITHENGE_EXPRESSION_INTEGER_DIVIDE: /* fallthrough */
	case BITHENGE_EXPRESSION_MODULO:
		return PRECEDENCE_MULTIPLY;
	case BITHENGE_EXPRESSION_LESS_THAN: /* fallthrough */
	case BITHENGE_EXPRESSION_LESS_THAN_OR_EQUAL: /* fallthrough */
	case BITHENGE_EXPRESSION_GREATER_THAN: /* fallthrough */
	case BITHENGE_EXPRESSION_GREATER_THAN_OR_EQUAL:
		return PRECEDENCE_COMPARE;
	case BITHENGE_EXPRESSION_EQUALS: /* fallthrough */
	case BITHENGE_EXPRESSION_NOT_EQUALS:
		return PRECEDENCE_EQUALS;
	case BITHENGE_EXPRESSION_AND: /* fallthrough */
	case BITHENGE_EXPRESSION_OR:
		return PRECEDENCE_AND;
	default:
		assert(false);
		return PRECEDENCE_NONE;
	}
}

static bithenge_expression_t *parse_term(state_t *state)
{
	errno_t rc;
	if (state->token == TOKEN_TRUE || state->token == TOKEN_FALSE) {
		bool val = state->token == TOKEN_TRUE;
		next_token(state);
		bithenge_node_t *node;
		rc = bithenge_new_boolean_node(&node, val);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}

		bithenge_expression_t *expr;
		rc = bithenge_const_expression(&expr, node);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}

		return expr;
	} else if (state->token == TOKEN_IN) {
		next_token(state);
		state->in_node_used = true;
		bithenge_expression_t *expr;
		rc = bithenge_in_node_expression(&expr);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
		return expr;
	} else if (state->token == TOKEN_INTEGER) {
		bithenge_int_t val = state->token_int;
		next_token(state);
		bithenge_node_t *node;
		rc = bithenge_new_integer_node(&node, val);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}

		bithenge_expression_t *expr;
		rc = bithenge_const_expression(&expr, node);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}

		return expr;
	} else if (state->token == TOKEN_IDENTIFIER) {
		int i;
		for (i = 0; i < state->num_params; i++)
			if (!str_cmp(state->parameter_names[i],
			    state->token_string))
				break;

		if (i == state->num_params) {
			syntax_error(state, "unknown identifier");
			return NULL;
		}

		next_token(state);

		bithenge_expression_t *expr;
		rc = bithenge_param_expression(&expr, i);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
		return expr;
	} else if (state->token == '.') {
		next_token(state);

		const char *id = expect_identifier(state);
		bithenge_node_t *key = NULL;
		bithenge_expression_t *expr = NULL;

		if (state->error == EOK) {
			rc = bithenge_new_string_node(&key, id, true);
			id = NULL;
			error_errno(state, rc);
		}

		if (state->error == EOK) {
			rc = bithenge_scope_member_expression(&expr, key);
			key = NULL;
			if (rc != EOK)
				expr = NULL;
			error_errno(state, rc);
		}

		if (state->error != EOK) {
			free((char *)id);
			bithenge_node_dec_ref(key);
			bithenge_expression_dec_ref(expr);
			return NULL;
		}

		return expr;
	} else if (state->token == '(') {
		next_token(state);
		bithenge_expression_t *expr = parse_expression(state);
		expect(state, ')');
		return expr;
	} else {
		syntax_error(state, "expression expected");
		return NULL;
	}
}

static bithenge_expression_t *parse_postfix_expression(state_t *state)
{
	errno_t rc;
	bithenge_expression_t *expr = parse_term(state);
	while (state->error == EOK) {
		if (state->token == '.') {
			next_token(state);

			const char *id = expect_identifier(state);

			if (state->error != EOK) {
				free((char *)id);
				bithenge_expression_dec_ref(expr);
				return NULL;
			}

			bithenge_node_t *key = NULL;
			rc = bithenge_new_string_node(&key, id, true);
			if (rc != EOK) {
				error_errno(state, rc);
				bithenge_expression_dec_ref(expr);
				return NULL;
			}

			bithenge_expression_t *key_expr;
			rc = bithenge_const_expression(&key_expr, key);
			if (rc != EOK) {
				error_errno(state, rc);
				bithenge_expression_dec_ref(expr);
				return NULL;
			}

			rc = bithenge_binary_expression(&expr,
			    BITHENGE_EXPRESSION_MEMBER, expr, key_expr);
			if (rc != EOK) {
				error_errno(state, rc);
				return NULL;
			}
		} else if (state->token == '[') {
			next_token(state);
			bithenge_expression_t *start = parse_expression(state);
			bool absolute_limit = false;
			if (state->token == ',' || state->token == ':') {
				absolute_limit = state->token == ':';
				next_token(state);
				bithenge_expression_t *limit = NULL;
				if (!(state->token == ']' && absolute_limit))
					limit = parse_expression(state);
				expect(state, ']');

				if (state->error != EOK) {
					bithenge_expression_dec_ref(expr);
					bithenge_expression_dec_ref(start);
					bithenge_expression_dec_ref(limit);
					return NULL;
				}
				rc = bithenge_subblob_expression(&expr, expr, start,
				    limit, absolute_limit);
				if (rc != EOK) {
					error_errno(state, rc);
					return NULL;
				}
			} else if (state->token == ']') {
				next_token(state);

				if (state->error != EOK) {
					bithenge_expression_dec_ref(expr);
					bithenge_expression_dec_ref(start);
					return NULL;
				}
				rc = bithenge_binary_expression(&expr,
				    BITHENGE_EXPRESSION_MEMBER, expr, start);
				if (rc != EOK) {
					error_errno(state, rc);
					return NULL;
				}
			} else {
				syntax_error(state, "expected ',', ':', or ']'");
				bithenge_expression_dec_ref(expr);
				bithenge_expression_dec_ref(start);
				return NULL;
			}
		} else {
			break;
		}
	}
	return expr;
}

static bithenge_expression_t *parse_expression_precedence(state_t *state,
    precedence_t prev_precedence)
{
	bithenge_expression_t *expr = parse_postfix_expression(state);
	while (state->error == EOK) {
		bithenge_binary_op_t op =
		    token_as_binary_operator(state->token);
		if (op == BITHENGE_EXPRESSION_INVALID_BINARY_OP)
			break;
		precedence_t precedence = binary_operator_precedence(op);
		if (precedence <= prev_precedence)
			break;
		next_token(state);

		bithenge_expression_t *expr2 =
		    parse_expression_precedence(state, precedence);
		if (state->error != EOK) {
			bithenge_expression_dec_ref(expr2);
			break;
		}
		errno_t rc = bithenge_binary_expression(&expr, op, expr, expr2);
		if (rc != EOK) {
			expr = NULL;
			error_errno(state, rc);
		}
	}
	if (state->error != EOK) {
		bithenge_expression_dec_ref(expr);
		expr = NULL;
	}
	return expr;
}

static bithenge_expression_t *parse_expression(state_t *state)
{
	return parse_expression_precedence(state, PRECEDENCE_NONE);
}



/* state->token must be TOKEN_IDENTIFIER when this is called. */
static bithenge_transform_t *parse_invocation(state_t *state)
{
	bithenge_transform_t *result = get_named_transform(state,
	    state->token_string);
	if (!result)
		syntax_error(state, "transform not found");
	next_token(state);

	bithenge_expression_t **params = NULL;
	int num_params = 0;
	if (state->token == '(') {
		next_token(state);
		while (state->error == EOK && state->token != ')') {
			if (num_params)
				expect(state, ',');
			params = state_realloc(state, params,
			    (num_params + 1)*sizeof(*params));
			if (state->error != EOK)
				break;
			params[num_params] = parse_expression(state);
			num_params++;
		}
		expect(state, ')');
	}

	/* TODO: show correct error position */
	if (state->error == EOK
	    && bithenge_transform_num_params(result) != num_params)
		syntax_error(state, "incorrect number of parameters before");

	if (state->error != EOK) {
		while (num_params--)
			bithenge_expression_dec_ref(params[num_params]);
		free(params);
		bithenge_transform_dec_ref(result);
		return NULL;
	}

	if (num_params) {
		errno_t rc = bithenge_param_wrapper(&result, result, params);
		if (rc != EOK) {
			error_errno(state, rc);
			result = NULL;
		}
	}

	return result;
}

/** Create a transform that just produces an empty node.
 * @param state The parser state.
 * @return The new transform, or NULL on error. */
static bithenge_transform_t *make_empty_transform(state_t *state)
{
	bithenge_node_t *node;
	errno_t rc = bithenge_new_empty_internal_node(&node);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}

	bithenge_expression_t *expr;
	rc = bithenge_const_expression(&expr, node);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}

	bithenge_transform_t *xform;
	rc = bithenge_inputless_transform(&xform, expr);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}

	return xform;
}

static bithenge_transform_t *parse_if(state_t *state, bool in_struct)
{
	expect(state, TOKEN_IF);
	expect(state, '(');
	bithenge_expression_t *expr = parse_expression(state);
	expect(state, ')');
	expect(state, '{');
	bithenge_transform_t *true_xform =
	    in_struct ? parse_struct(state) : parse_transform(state);
	expect(state, '}');

	bithenge_transform_t *false_xform = NULL;
	if (state->token == TOKEN_ELSE) {
		next_token(state);
		expect(state, '{');
		false_xform =
		    in_struct ? parse_struct(state) : parse_transform(state);
		expect(state, '}');
	} else {
		if (in_struct)
			false_xform = make_empty_transform(state);
		else
			syntax_error(state, "else expected");
	}

	if (state->error != EOK) {
		bithenge_expression_dec_ref(expr);
		bithenge_transform_dec_ref(true_xform);
		bithenge_transform_dec_ref(false_xform);
		return NULL;
	}

	bithenge_transform_t *if_xform;
	errno_t rc = bithenge_if_transform(&if_xform, expr, true_xform,
	    false_xform);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}
	return if_xform;
}

static bithenge_transform_t *parse_switch(state_t *state, bool in_struct)
{
	expect(state, TOKEN_SWITCH);
	expect(state, '(');
	bithenge_expression_t *ref_expr = parse_expression(state);
	expect(state, ')');
	expect(state, '{');
	int num = 0;
	bithenge_expression_t **exprs = NULL;
	bithenge_transform_t **xforms = NULL;
	while (state->error == EOK && state->token != '}') {
		bithenge_expression_t *expr;
		if (state->token == TOKEN_ELSE) {
			next_token(state);
			bithenge_node_t *node;
			errno_t rc = bithenge_new_boolean_node(&node, true);
			if (rc != EOK) {
				error_errno(state, rc);
				break;
			}
			rc = bithenge_const_expression(&expr, node);
			if (rc != EOK) {
				error_errno(state, rc);
				break;
			}
		} else {
			expr = parse_expression(state);
			if (state->error == EOK) {
				bithenge_expression_inc_ref(ref_expr);
				errno_t rc = bithenge_binary_expression(&expr,
				    BITHENGE_EXPRESSION_EQUALS, ref_expr,
				    expr);
				if (rc != EOK) {
					error_errno(state, rc);
					break;
				}
			}
		}

		expect(state, ':');
		bithenge_transform_t *xform;
		if (in_struct) {
			expect(state, '{');
			xform = parse_struct(state);
			expect(state, '}');
		} else
			xform = parse_transform(state);
		expect(state, ';');

		exprs = state_realloc(state, exprs,
		    sizeof(*exprs) * (num + 1));
		xforms = state_realloc(state, xforms,
		    sizeof(*xforms) * (num + 1));
		if (state->error != EOK) {
			bithenge_expression_dec_ref(expr);
			bithenge_transform_dec_ref(xform);
			break;
		}

		exprs[num] = expr;
		xforms[num] = xform;
		num++;
	}
	bithenge_expression_dec_ref(ref_expr);

	bithenge_transform_t *switch_xform = &bithenge_invalid_transform;
	bithenge_transform_inc_ref(switch_xform);
	while (state->error == EOK && num >= 1) {
		num--;
		errno_t rc = bithenge_if_transform(&switch_xform, exprs[num],
		    xforms[num], switch_xform);
		if (rc != EOK) {
			switch_xform = NULL;
			error_errno(state, rc);
		}
	}

	while (num >= 1) {
		num--;
		bithenge_expression_dec_ref(exprs[num]);
		bithenge_transform_dec_ref(xforms[num]);
	}
	free(exprs);
	free(xforms);

	expect(state, '}');
	return switch_xform;
}

static bithenge_transform_t *parse_repeat(state_t *state)
{
	expect(state, TOKEN_REPEAT);
	bithenge_expression_t *expr = NULL;
	if (state->token == '(') {
		next_token(state);
		expr = parse_expression(state);
		expect(state, ')');
	}
	expect(state, '{');
	bithenge_transform_t *xform = parse_transform(state);
	expect(state, '}');

	if (state->error != EOK) {
		bithenge_expression_dec_ref(expr);
		bithenge_transform_dec_ref(xform);
		return NULL;
	}

	bithenge_transform_t *repeat_xform;
	errno_t rc = bithenge_repeat_transform(&repeat_xform, xform, expr);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}
	return repeat_xform;
}

static bithenge_transform_t *parse_do_while(state_t *state)
{
	expect(state, TOKEN_DO);
	expect(state, '{');
	bithenge_transform_t *xform = parse_transform(state);
	expect(state, '}');
	expect(state, TOKEN_WHILE);
	expect(state, '(');
	bithenge_expression_t *expr = parse_expression(state);
	expect(state, ')');

	if (state->error != EOK) {
		bithenge_expression_dec_ref(expr);
		bithenge_transform_dec_ref(xform);
		return NULL;
	}

	bithenge_transform_t *do_while_xform;
	errno_t rc = bithenge_do_while_transform(&do_while_xform, xform, expr);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}
	return do_while_xform;
}

static bithenge_transform_t *parse_partial(state_t *state)
{
	expect(state, TOKEN_PARTIAL);
	bithenge_transform_t *offset_xform = NULL;
	if (state->token == '(') {
		next_token(state);
		bithenge_expression_t *offset = parse_expression(state);
		expect(state, ')');

		bithenge_expression_t *in_expr;
		errno_t rc = bithenge_in_node_expression(&in_expr);
		if (rc != EOK)
			error_errno(state, rc);
		if (state->error != EOK) {
			bithenge_expression_dec_ref(offset);
			return NULL;
		}

		rc = bithenge_subblob_expression(&offset, in_expr, offset,
		    NULL, true);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}

		rc = bithenge_expression_transform(&offset_xform, offset);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
	}
	expect(state, '{');
	bithenge_transform_t *xform = parse_transform(state);
	expect(state, '}');
	if (state->error != EOK) {
		bithenge_transform_dec_ref(offset_xform);
		bithenge_transform_dec_ref(xform);
		return NULL;
	}

	errno_t rc = bithenge_partial_transform(&xform, xform);
	if (rc != EOK) {
		error_errno(state, rc);
		bithenge_transform_dec_ref(offset_xform);
		return NULL;
	}

	if (offset_xform) {
		bithenge_transform_t **xforms = malloc(2 * sizeof(*xforms));
		if (!xforms) {
			error_errno(state, ENOMEM);
			bithenge_transform_dec_ref(xform);
			bithenge_transform_dec_ref(offset_xform);
			return NULL;
		}
		xforms[0] = xform;
		xforms[1] = offset_xform;
		rc = bithenge_new_composed_transform(&xform, xforms, 2);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
	}

	return xform;
}

/* The TOKEN_STRUCT and '{' must already have been skipped. */
static bithenge_transform_t *parse_struct(state_t *state)
{
	size_t num = 0;
	bithenge_named_transform_t *subxforms;
	/* We keep an extra space for the {NULL, NULL} terminator. */
	subxforms = state_malloc(state, sizeof(*subxforms));
	while (state->error == EOK && state->token != '}') {
		if (state->token == TOKEN_IF) {
			subxforms[num].transform = parse_if(state, true);
			subxforms[num].name = NULL;
		} else if (state->token == TOKEN_SWITCH) {
			subxforms[num].transform = parse_switch(state, true);
			subxforms[num].name = NULL;
		} else {
			if (state->token == '.') {
				next_token(state);
				subxforms[num].name = expect_identifier(state);
			} else {
				subxforms[num].name = NULL;
			}
			expect(state, TOKEN_LEFT_ARROW);
			subxforms[num].transform = parse_transform(state);
			expect(state, ';');
		}
		num++;
		subxforms = state_realloc(state, subxforms,
		    (num + 1)*sizeof(*subxforms));
	}

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
	errno_t rc = bithenge_new_struct(&result, subxforms);
	if (rc != EOK) {
		error_errno(state, rc);
		return NULL;
	}
	return result;
}

/** Parse a transform without composition.
 * @return The parsed transform, or NULL if an error occurred. */
static bithenge_transform_t *parse_transform_no_compose(state_t *state)
{
	if (state->token == '(') {
		next_token(state);
		state->in_node_used = false;
		bithenge_expression_t *expr = parse_expression(state);
		expect(state, ')');
		if (state->error != EOK) {
			bithenge_expression_dec_ref(expr);
			return NULL;
		}

		bithenge_transform_t *xform;
		errno_t rc;
		if (state->in_node_used)
			rc = bithenge_expression_transform(&xform, expr);
		else
			rc = bithenge_inputless_transform(&xform, expr);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
		return xform;
	} else if (state->token == TOKEN_DO) {
		return parse_do_while(state);
	} else if (state->token == TOKEN_IDENTIFIER) {
		return parse_invocation(state);
	} else if (state->token == TOKEN_IF) {
		return parse_if(state, false);
	} else if (state->token == TOKEN_PARTIAL) {
		return parse_partial(state);
	} else if (state->token == TOKEN_REPEAT) {
		return parse_repeat(state);
	} else if (state->token == TOKEN_STRUCT) {
		next_token(state);
		expect(state, '{');
		bithenge_transform_t *xform = parse_struct(state);
		expect(state, '}');
		return xform;
	} else if (state->token == TOKEN_SWITCH) {
		return parse_switch(state, false);
	} else {
		syntax_error(state, "unexpected (transform expected)");
		return NULL;
	}
}

/** Parse a transform.
 * @return The parsed transform, or NULL if an error occurred. */
static bithenge_transform_t *parse_transform(state_t *state)
{
	bithenge_transform_t *result = parse_transform_no_compose(state);
	bithenge_transform_t **xforms = NULL;
	size_t num = 1;
	while (state->token == TOKEN_LEFT_ARROW) {
		expect(state, TOKEN_LEFT_ARROW);
		xforms = state_realloc(state, xforms,
		    (num + 1) * sizeof(*xforms));
		if (state->error != EOK)
			break;
		xforms[num++] = parse_transform_no_compose(state);
	}
	if (state->error != EOK) {
		while (xforms && num > 1)
			bithenge_transform_dec_ref(xforms[--num]);
		free(xforms);
		bithenge_transform_dec_ref(result);
		return NULL;
	}
	if (xforms) {
		xforms[0] = result;
		errno_t rc = bithenge_new_composed_transform(&result, xforms, num);
		if (rc != EOK) {
			error_errno(state, rc);
			return NULL;
		}
	}
	return result;
}

/** Parse a definition. */
static void parse_definition(state_t *state)
{
	expect(state, TOKEN_TRANSFORM);
	char *name = expect_identifier(state);

	if (state->token == '(') {
		next_token(state);
		while (state->error == EOK && state->token != ')') {
			if (state->num_params)
				expect(state, ',');
			state->parameter_names = state_realloc(state,
			    state->parameter_names,
			    (state->num_params + 1)*sizeof(*state->parameter_names));
			if (state->error != EOK)
				break;
			state->parameter_names[state->num_params++] =
			    expect_identifier(state);
		}
		expect(state, ')');
	}

	bithenge_transform_t *barrier = NULL;
	if (state->error == EOK) {
		errno_t rc = bithenge_new_barrier_transform(&barrier,
		    state->num_params);
		if (rc != EOK) {
			barrier = NULL;
			error_errno(state, rc);
		}
	}

	add_named_transform(state, barrier, name);

	expect(state, '=');
	bithenge_transform_t *xform = parse_transform(state);
	expect(state, ';');

	if (state->error == EOK) {
		errno_t rc = bithenge_barrier_transform_set_subtransform(barrier,
		    xform);
		xform = NULL;
		if (rc != EOK)
			error_errno(state, rc);
	}

	if (state->error != EOK)
		bithenge_transform_dec_ref(xform);

	for (int i = 0; i < state->num_params; i++)
		free(state->parameter_names[i]);
	free(state->parameter_names);
	state->parameter_names = NULL;
	state->num_params = 0;
}

/** Initialize the state. */
static void state_init(state_t *state, const char *filename)
{
	state->error = EOK;
	state->transform_list = NULL;
	state->parameter_names = NULL;
	state->num_params = 0;
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
	if (state->file)
		fclose(state->file);
	transform_list_t *entry = state->transform_list;
	while (entry) {
		transform_list_t *next = entry->next;
		free(entry->name);
		bithenge_transform_dec_ref(entry->transform);
		free(entry);
		entry = next;
	}
	for (int i = 0; i < state->num_params; i++)
		free(state->parameter_names[i]);
	free(state->parameter_names);
}

/** Parse a script file.
 * @param filename The name of the script file.
 * @param[out] out Stores the "main" transform.
 * @return EOK on success, EINVAL on syntax error, or an error code from
 * errno.h. */
errno_t bithenge_parse_script(const char *filename, bithenge_transform_t **out)
{
	state_t state;
	state_init(&state, filename);
	while (state.error == EOK && state.token != TOKEN_EOF)
		parse_definition(&state);
	*out = get_named_transform(&state, "main");
	errno_t rc = state.error;
	state_destroy(&state);
	if (rc == EOK && !*out) {
		fprintf(stderr, "no \"main\" transform\n");
		rc = EINVAL;
	}
	return rc;
}

/** @}
 */
