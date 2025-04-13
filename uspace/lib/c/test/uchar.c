/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#include <mem.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <uchar.h>

PCUT_INIT;

PCUT_TEST_SUITE(uchar);

PCUT_TEST(mbrtoc16)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	size_t ret;
	const char *s = u8"a𐎣";
	char16_t out[] = u"AAAAA";
	const char16_t expected[] = u"a𐎣";

	ret = mbrtoc16(&out[0], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(expected[0], out[0]);

	s += ret;

	ret = mbrtoc16(&out[1], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(expected[1], out[1]);

	s += ret;

	ret = mbrtoc16(&out[2], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(-3, ret);
	PCUT_ASSERT_INT_EQUALS(expected[2], out[2]);

	ret = mbrtoc16(&out[3], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(0, ret);
	PCUT_ASSERT_INT_EQUALS(expected[3], out[3]);
}

PCUT_TEST(c16rtomb)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	const char16_t in[] = u"aβℷ𐎣";
	char out[] = "AAAAAAAAAAAAAAAAA";

	char *s = out;

	for (size_t i = 0; i < sizeof(in) / sizeof(char16_t); i++) {
		s += c16rtomb(s, in[i], &mbstate);
	}

	const char expected[] = u8"aβℷ𐎣";
	PCUT_ASSERT_STR_EQUALS(expected, out);
	PCUT_ASSERT_INT_EQUALS(s - out, sizeof(expected));
}

PCUT_TEST(mbrtoc32)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	size_t ret;
	char32_t c = 0;
	const char *s = u8"aβℷ𐎣";

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'a');

	s += ret;

	ret = mbrtoc32(&c, s, 1, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, -2);
	PCUT_ASSERT_INT_EQUALS(c, U'a');

	s += 1;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'β');

	s += ret;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 3);
	PCUT_ASSERT_INT_EQUALS(c, U'ℷ');

	s += ret;

	ret = mbrtoc32(&c, s, 3, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, -2);
	PCUT_ASSERT_INT_EQUALS(c, U'ℷ');

	s += 3;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'𐎣');

	s += ret;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 0);
	PCUT_ASSERT_INT_EQUALS(c, 0);
}

PCUT_TEST(c32rtomb)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	char out[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	char *s = out;

	_Static_assert(sizeof(out) > 5 * MB_CUR_MAX);

	s += c32rtomb(s, U'a', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 1);

	s += c32rtomb(s, U'β', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 3);

	s += c32rtomb(s, U'ℷ', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 6);

	s += c32rtomb(s, U'𐎣', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 10);

	s += c32rtomb(s, 0, &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 11);

	const char expected[] = u8"aβℷ𐎣";
	PCUT_ASSERT_INT_EQUALS(memcmp(out, expected, sizeof(expected)), 0);
}

PCUT_EXPORT(uchar);
