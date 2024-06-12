/*
 * Copyright (c) 2019 Matthieu Riolo
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

#include <pcut/pcut.h>
#include <uuid.h>
#include <stdbool.h>
#include <stdio.h>
#include <str.h>
#include <ctype.h>

static size_t MAX_SUB_TESTS = 10;
static size_t uuids_len = 6;
static const char *uuids[] = {
	/* uppercase */
	"F81163AE-299A-4DA2-BED1-0E096C59F3AB",
	"4A0CE2A3-FD1C-4951-972E-AAA13A703078",
	"69C7DB62-8309-4C58-831B-8C4E4161E8AC",

	/* lower case */
	"c511bf24-70cb-422e-933b-2a74ab699a56",
	"7b1abd05-456f-4661-ab62-917685069343",
	"5b00f76b-4a16-4dce-a1fc-b78c60324d89"
};

static bool uuid_valid(uuid_t uuid)
{
	if (!((uuid.time_hi_and_version & 0xf000) & 0x4000)) {
		return false;
	}

	int f = (uuid.clock_seq_hi_and_reserved & 0x80) || (uuid.clock_seq_hi_and_reserved & 0x90);
	f = f || (uuid.clock_seq_hi_and_reserved & 0xA0) || (uuid.clock_seq_hi_and_reserved & 0xB0);
	if (!f) {
		return false;
	}

	return true;
}

PCUT_INIT;

PCUT_TEST_SUITE(uuid);

PCUT_TEST(uuid_generate)
{
	uuid_t uuid;
	size_t i;

	for (i = 0; i < MAX_SUB_TESTS; i++) {
		errno_t ret = uuid_generate(&uuid);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_TRUE(uuid_valid(uuid));
	}
}

PCUT_TEST(uuid_parse)
{
	uuid_t uuid;
	errno_t ret;

	for (size_t i = 0; i < uuids_len; i++) {
		ret = uuid_parse(uuids[i], &uuid, NULL);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_TRUE(uuid_valid(uuid));
	}
}

PCUT_TEST(uuid_parse_in_text)
{
	uuid_t uuid;
	errno_t ret;
	const char *endptr;
	const char *uuid_in_text = "7b1abd05-456f-4661-ab62-917685069343hello world!";

	ret = uuid_parse(uuid_in_text, &uuid, &endptr);

	PCUT_ASSERT_ERRNO_VAL(EOK, ret);
	PCUT_ASSERT_TRUE(uuid_valid(uuid));
	PCUT_ASSERT_STR_EQUALS("hello world!", endptr);
}

PCUT_TEST(uuid_format_generated)
{
	uuid_t uuid;
	size_t i;
	char *rstr;

	for (i = 0; i < MAX_SUB_TESTS; i++) {
		errno_t ret = uuid_generate(&uuid);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_TRUE(uuid_valid(uuid));

		ret = uuid_format(&uuid, &rstr, true);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_INT_EQUALS('\0', rstr[37]);
		PCUT_ASSERT_INT_EQUALS(36, str_length(rstr));
		PCUT_ASSERT_INT_EQUALS('4', rstr[14]);

		int f = rstr[19] == '8' || rstr[19] == '9';
		f = f || toupper(rstr[19]) == 'A' || toupper(rstr[19]) == 'B';
		PCUT_ASSERT_TRUE(f);

		free(rstr);
	}
}

PCUT_TEST(uuid_format_parsed)
{
	uuid_t uuid;
	char *rstr;
	errno_t ret;

	for (size_t i = 0; i < uuids_len; i++) {
		ret = uuid_parse(uuids[i], &uuid, NULL);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_TRUE(uuid_valid(uuid));

		ret = uuid_format(&uuid, &rstr, true);
		PCUT_ASSERT_ERRNO_VAL(EOK, ret);
		PCUT_ASSERT_INT_EQUALS('\0', rstr[37]);
		PCUT_ASSERT_INT_EQUALS(36, str_length(rstr));
		PCUT_ASSERT_INT_EQUALS(0, str_casecmp(uuids[i], rstr));

		free(rstr);
	}
}

PCUT_EXPORT(uuid);
