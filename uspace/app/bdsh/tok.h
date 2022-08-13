/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TOK_H
#define TOK_H

typedef enum {
	TOKTYPE_TEXT,
	TOKTYPE_PIPE,
	TOKTYPE_SPACE
} token_type_t;

typedef struct {
	char *text;
	size_t byte_start;
	size_t char_start;
	size_t byte_length;
	size_t char_length;
	token_type_t type;
} token_t;

typedef struct {
	char *in;
	size_t in_offset;
	size_t last_in_offset;
	size_t in_char_offset;
	size_t last_in_char_offset;

	char *outbuf;
	size_t outbuf_offset;
	size_t outbuf_size;
	size_t outbuf_last_start;

	token_t *outtok;
	token_type_t current_type;
	size_t outtok_offset;
	size_t outtok_size;
} tokenizer_t;

extern errno_t tok_init(tokenizer_t *, char *, token_t *, size_t);
extern void tok_fini(tokenizer_t *);
extern errno_t tok_tokenize(tokenizer_t *, size_t *);

#endif
