/*
 * Copyright (c) 2011 Martin Sucha
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
