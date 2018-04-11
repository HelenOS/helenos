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

#include <stdio.h>
#include <str.h>
#include <pcut/pcut.h>

#define BUFFER_SIZE 256

#define SET_BUFFER(str) snprintf(buffer, BUFFER_SIZE, "%s", str)
#define EQ(expected, value) PCUT_ASSERT_STR_EQUALS(expected, value)



PCUT_INIT;

PCUT_TEST_SUITE(str);

static char buffer[BUFFER_SIZE];

PCUT_TEST_BEFORE
{
	memset(buffer, 0, BUFFER_SIZE);
}


PCUT_TEST(rtrim)
{
	SET_BUFFER("foobar");
	str_rtrim(buffer, ' ');
	EQ("foobar", buffer);

	SET_BUFFER("  foobar  ");
	str_rtrim(buffer, ' ');
	EQ("  foobar", buffer);

	SET_BUFFER("  ššš  ");
	str_rtrim(buffer, ' ');
	EQ("  ššš", buffer);

	SET_BUFFER("ššAAAšš");
	str_rtrim(buffer, L'š');
	EQ("ššAAA", buffer);
}

PCUT_TEST(ltrim)
{
	SET_BUFFER("foobar");
	str_ltrim(buffer, ' ');
	EQ("foobar", buffer);

	SET_BUFFER("  foobar  ");
	str_ltrim(buffer, ' ');
	EQ("foobar  ", buffer);

	SET_BUFFER("  ššš  ");
	str_ltrim(buffer, ' ');
	EQ("ššš  ", buffer);

	SET_BUFFER("ššAAAšš");
	str_ltrim(buffer, L'š');
	EQ("AAAšš", buffer);
}


PCUT_EXPORT(str);
