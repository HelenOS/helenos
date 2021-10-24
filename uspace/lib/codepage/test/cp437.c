/*
 * Copyright (c) 2021 Jiri Svoboda
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

#include <codepage/cp437.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(cp437);

/** Encode(decode) == identity */
PCUT_TEST(decode_encode)
{
	unsigned code;
	char32_t c;
	uint8_t cdec;
	errno_t rc;

	for (code = 0; code < 256; code++) {
		c = cp437_decode(code);
		rc = cp437_encode(c, &cdec);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_EQUALS(code, cdec);
	}
}

/** Test limit encoding cases */
PCUT_TEST(encode_cases)
{
	uint8_t code;
	errno_t rc;

	/* Zero maps to zero */
	rc = cp437_encode(0, &code);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(0x00, code);

	/* Last entry in the 0x0000-0x03ff table is unmapped */
	rc = cp437_encode(0x3ff, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);

	/* 0x0400 is out of range of first table */
	rc = cp437_encode(0x400, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);

	/* 0x1fff is before start of 0x2000-0x26ff table */
	rc = cp437_encode(0x1fff, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);

	/* First entry in 0x2000-0x26ff table is unmapped */
	rc = cp437_encode(0x2000, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);

	/* Last entry in 0x2000-0x26ff table is unmapped */
	rc = cp437_encode(0x26ff, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);

	/* 0x2700 is beyond the end of 0x2000-0x26ff table */
	rc = cp437_encode(0x2700, &code);
	PCUT_ASSERT_ERRNO_VAL(EDOM, rc);
}

PCUT_EXPORT(cp437);
