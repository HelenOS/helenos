/*
 * Copyright (c) 2014 Vojtech Horky
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

#include <errno.h>
#include <str.h>
#include <stdlib.h>

#include "../tok.h"
#include <pcut/pcut.h>

PCUT_INIT;

#define MAX_TOKENS 32
#define MAX_INPUT 256
static tokenizer_t tokenizer;
static token_t tokens[MAX_TOKENS];
static char input_buffer[MAX_INPUT];

/* Tokenize the input, asserts that number of tokens is okay. */
static void prepare(const char *input, size_t expected_token_count)
{
	str_cpy(input_buffer, MAX_INPUT, input);

	errno_t rc = tok_init(&tokenizer, input_buffer, tokens, MAX_TOKENS);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	size_t token_count;
	rc = tok_tokenize(&tokenizer, &token_count);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);


	PCUT_ASSERT_INT_EQUALS(expected_token_count, token_count);
}


#define ASSERT_TOKEN(index, token_type, token_text) \
	do { \
		PCUT_ASSERT_INT_EQUALS(token_type, tokens[index].type); \
		PCUT_ASSERT_STR_EQUALS(token_text, tokens[index].text); \
	} while (0)



PCUT_TEST_SUITE(tokenizer);

PCUT_TEST_AFTER
{
	/* Destroy the tokenizer. */
	tok_fini(&tokenizer);
}


PCUT_TEST(empty_input)
{
	prepare("", 0);
}

PCUT_TEST(only_spaces)
{
	prepare("   ", 1);

	ASSERT_TOKEN(0, TOKTYPE_SPACE, "   ");
}

PCUT_TEST(two_text_tokens)
{
	prepare("alpha  bravo", 3);

	ASSERT_TOKEN(0, TOKTYPE_TEXT, "alpha");
	ASSERT_TOKEN(1, TOKTYPE_SPACE, "  ");
	ASSERT_TOKEN(2, TOKTYPE_TEXT, "bravo");
}



PCUT_MAIN();
