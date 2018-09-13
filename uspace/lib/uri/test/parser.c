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

#include <pcut/pcut.h>
#include <str.h>
#include "../uri.h"

typedef struct {
	const char *scheme;
	const char *user_info;
	const char *user_credential;
	const char *host;
	const char *port;
	const char *path;
	const char *query;
	const char *fragment;
} const_uri_t;

static const_uri_t expected_uri;
static uri_t *parsed_uri;

/*
 * In the following macros we assume that the expected and the parsed
 * URI are in the global variables declared above.
 */

#define CHECK(item) \
	PCUT_ASSERT_STR_EQUALS_OR_NULL(expected_uri.item, parsed_uri->item)

#define PARSE_AND_CHECK(the_uri) \
	do { \
		parsed_uri = uri_parse(the_uri); \
		PCUT_ASSERT_NOT_NULL(parsed_uri); \
		PCUT_ASSERT_INT_EQUALS(0, !uri_validate(parsed_uri)); \
		CHECK(scheme); \
		CHECK(user_info); \
		CHECK(user_credential); \
		CHECK(host); \
		CHECK(port); \
		CHECK(path); \
		CHECK(query); \
		CHECK(fragment); \
	} while (0)

PCUT_INIT;

PCUT_TEST_SUITE(uri_parse);

PCUT_TEST_BEFORE
{
	parsed_uri = NULL;
	expected_uri.scheme = NULL;
	expected_uri.user_info = NULL;
	expected_uri.user_credential = NULL;
	expected_uri.host = NULL;
	expected_uri.port = NULL;
	expected_uri.path = "";
	expected_uri.query = NULL;
	expected_uri.fragment = NULL;
}

PCUT_TEST_AFTER
{
	if (parsed_uri != NULL) {
		uri_destroy(parsed_uri);
		parsed_uri = NULL;
	}
}

PCUT_TEST(only_hostname)
{
	expected_uri.scheme = "http";
	expected_uri.host = "localhost";

	PARSE_AND_CHECK("http://localhost");
}

PCUT_TEST(hostname_with_user)
{
	expected_uri.scheme = "http";
	expected_uri.host = "localhost";
	expected_uri.user_info = "user";

	PARSE_AND_CHECK("http://user@localhost");
}

PCUT_TEST(hostname_with_user_and_password)
{
	expected_uri.scheme = "https";
	expected_uri.host = "localhost";
	expected_uri.user_info = "user";
	expected_uri.user_credential = "password";

	PARSE_AND_CHECK("https://user:password@localhost");
}

PCUT_TEST(path_specification)
{
	expected_uri.scheme = "http";
	expected_uri.host = "localhost";
	expected_uri.path = "/alpha";

	PARSE_AND_CHECK("http://localhost/alpha");
}

PCUT_TEST(with_fragment)
{
	expected_uri.scheme = "http";
	expected_uri.host = "localhost";
	expected_uri.path = "/alpha";
	expected_uri.fragment = "fragment-name";

	PARSE_AND_CHECK("http://localhost/alpha#fragment-name");
}

PCUT_EXPORT(uri_parse);
