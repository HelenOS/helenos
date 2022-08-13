/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
