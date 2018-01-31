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

#include <str.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "tok.h"

/* Forward declarations of static functions */
static wchar_t tok_get_char(tokenizer_t *);
static wchar_t tok_look_char(tokenizer_t *);
static errno_t tok_push_char(tokenizer_t *, wchar_t);
static errno_t tok_push_token(tokenizer_t *);
static bool tok_pending_chars(tokenizer_t *);
static errno_t tok_finish_string(tokenizer_t *);
static void tok_start_token(tokenizer_t *, token_type_t);

/** Initialize the token parser
 * 
 * @param tok the tokenizer structure to initialize
 * @param input the input string to tokenize
 * @param out_tokens array of strings where to store the result
 * @param max_tokens number of elements of the out_tokens array
 */
errno_t tok_init(tokenizer_t *tok, char *input, token_t *out_tokens,
    size_t max_tokens)
{	
	tok->in = input;
	tok->in_offset = 0;
	tok->last_in_offset = 0;
	tok->in_char_offset = 0;
	tok->last_in_char_offset = 0;
	
	tok->outtok = out_tokens;
	tok->outtok_offset = 0;
	tok->outtok_size = max_tokens;
	
	/* Prepare a buffer where all the token strings will be stored */
	size_t len = str_size(input) + max_tokens + 1;
	char *tmp = malloc(len);
	
	if (tmp == NULL) {
		return ENOMEM;
	}
	
	tok->outbuf = tmp;
	tok->outbuf_offset = 0;
	tok->outbuf_size = len;
	tok->outbuf_last_start = 0;
	
	return EOK;
}

/** Finalize the token parser */
void tok_fini(tokenizer_t *tok)
{
	if (tok->outbuf != NULL) {
		free(tok->outbuf);
	}
}

/** Tokenize the input string into the tokens */
errno_t tok_tokenize(tokenizer_t *tok, size_t *tokens_length)
{
	errno_t rc;
	wchar_t next_char;
	
	/* Read the input line char by char and append tokens */
	while ((next_char = tok_look_char(tok)) != 0) {
		if (next_char == ' ') {
			/* Push the token if there is any.
			 * There may not be any pending char for a token in case
			 * there are several spaces in the input.
			 */
			if (tok_pending_chars(tok)) {
				rc = tok_push_token(tok);
				if (rc != EOK) {
					return rc;
				}
			}
			tok_start_token(tok, TOKTYPE_SPACE);
			/* Eat all the spaces */
			while (tok_look_char(tok) == ' ') {
				tok_push_char(tok, tok_get_char(tok));
			}
			tok_push_token(tok);
			
		}
		else if (next_char == '|') {
			/* Pipes are tokens that are delimiters and should be
			 * output as a separate token
			 */
			if (tok_pending_chars(tok)) {
				rc = tok_push_token(tok);
				if (rc != EOK) {
					return rc;
				}
			}
			
			tok_start_token(tok, TOKTYPE_PIPE);
			
			rc = tok_push_char(tok, tok_get_char(tok));
			if (rc != EOK) {
				return rc;
			}
			
			rc = tok_push_token(tok);
			if (rc != EOK) {
				return rc;
			}
		}
		else if (next_char == '\'') {
			/* A string starts with a quote (') and ends again with a quote.
			 * A literal quote is written as ''
			 */
			tok_start_token(tok, TOKTYPE_TEXT);
			/* Eat the quote */
			tok_get_char(tok);
			rc = tok_finish_string(tok);
			if (rc != EOK) {
				return rc;
			}
		}
		else {
			if (!tok_pending_chars(tok)) {
				tok_start_token(tok, TOKTYPE_TEXT);
			}
			/* If we are handling any other character, just append it to
			 * the current token.
			 */
			rc = tok_push_char(tok, tok_get_char(tok));
			if (rc != EOK) {
				return rc;
			}
		}
	}
	
	/* Push the last token */
	if (tok_pending_chars(tok)) {
		rc = tok_push_token(tok);
		if (rc != EOK) {
			return rc;
		}
	}
	
	*tokens_length = tok->outtok_offset;
	
	return EOK;
}

/** Finish tokenizing an opened string */
errno_t tok_finish_string(tokenizer_t *tok)
{
	errno_t rc;
	wchar_t next_char;
	
	while ((next_char = tok_look_char(tok)) != 0) {
		if (next_char == '\'') {
			/* Eat the quote */
			tok_get_char(tok);
			if (tok_look_char(tok) == '\'') {
				/* Encode a single literal quote */
				rc = tok_push_char(tok, '\'');
				if (rc != EOK) {
					return rc;
				}
				
				/* Swallow the additional one in the input */
				tok_get_char(tok);
			}
			else {
				/* The string end */
				return tok_push_token(tok);
			}
		}
		else {
			rc = tok_push_char(tok, tok_get_char(tok));
			if (rc != EOK) {
				return rc;
			}
		}
	}
	
	/* If we are here, the string run to the end without being closed */
	return EINVAL;
}

/** Get a char from input, advancing the input position */
wchar_t tok_get_char(tokenizer_t *tok)
{
	tok->in_char_offset++;
	return str_decode(tok->in, &tok->in_offset, STR_NO_LIMIT);
}

/** Get a char from input, while staying on the same input position */
wchar_t tok_look_char(tokenizer_t *tok)
{
	size_t old_offset = tok->in_offset;
	size_t old_char_offset = tok->in_char_offset;
	wchar_t ret = tok_get_char(tok);
	tok->in_offset = old_offset;
	tok->in_char_offset = old_char_offset;
	return ret;
}

/** Append a char to the end of the current token */
errno_t tok_push_char(tokenizer_t *tok, wchar_t ch)
{
	return chr_encode(ch, tok->outbuf, &tok->outbuf_offset, tok->outbuf_size);
}

void tok_start_token(tokenizer_t *tok, token_type_t type)
{
	tok->current_type = type;
}

/** Push the current token to the output array */
errno_t tok_push_token(tokenizer_t *tok)
{
	if (tok->outtok_offset >= tok->outtok_size) {
		return EOVERFLOW;
	}
	
	if (tok->outbuf_offset >= tok->outbuf_size) {
		return EOVERFLOW;
	}
	
	tok->outbuf[tok->outbuf_offset++] = 0;
	token_t *tokinfo = &tok->outtok[tok->outtok_offset++];
	tokinfo->type = tok->current_type;
	tokinfo->text = tok->outbuf + tok->outbuf_last_start;
	tokinfo->byte_start = tok->last_in_offset;
	tokinfo->byte_length = tok->in_offset - tok->last_in_offset;
	tokinfo->char_start = tok->last_in_char_offset;
	tokinfo->char_length = tok->in_char_offset - tok->last_in_char_offset;
	tok->outbuf_last_start = tok->outbuf_offset;
	
	/* We have consumed the first char of the next token already */
	tok->last_in_offset = tok->in_offset;
	tok->last_in_char_offset = tok->in_char_offset;
	
	return EOK;
}

/** Return true, if the current token is not empty */
bool tok_pending_chars(tokenizer_t *tok)
{
	assert(tok->outbuf_offset >= tok->outbuf_last_start);
	return (tok->outbuf_offset != tok->outbuf_last_start);
}
