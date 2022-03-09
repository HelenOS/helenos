/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * DATA, OR PROFITS; OR BUSINESS INTvvhhzccgggrERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <pcut/pcut.h>
#include <stdlib.h>
#include <str.h>
#include <uchar.h>
#include <ui/accel.h>

PCUT_INIT;

PCUT_TEST_SUITE(accel);

/** ui_accel_process() */
PCUT_TEST(process)
{
	errno_t rc;
	char *buf;
	char *endptr;
	char *bp1;
	char *bp2;

	/* Cases where one string is produced */

	rc = ui_accel_process("", &buf, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("", buf);
	PCUT_ASSERT_EQUALS(buf + 1, endptr);
	free(buf);

	rc = ui_accel_process("Hello", &buf, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("Hello", buf);
	PCUT_ASSERT_EQUALS(buf + str_size("Hello") + 1, endptr);
	free(buf);

	rc = ui_accel_process("~~Hello~~", &buf, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("~Hello~", buf);
	PCUT_ASSERT_EQUALS(buf + str_size("~Hello~") + 1, endptr);
	free(buf);

	/* Three strings are produced (the first is empty) */

	rc = ui_accel_process("~H~ello", &buf, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("", buf);

	bp1 = buf + str_size(buf) + 1;
	PCUT_ASSERT_STR_EQUALS("H", bp1);

	bp2 = bp1 + str_size(bp1) + 1;
	PCUT_ASSERT_STR_EQUALS("ello", bp2);

	PCUT_ASSERT_EQUALS(bp2 + str_size("ello") + 1, endptr);
	free(buf);
}

/** ui_accel_get() */
PCUT_TEST(get)
{
	char32_t a;

	a = ui_accel_get("");
	PCUT_ASSERT_EQUALS('\0', a);

	a = ui_accel_get("Hello");
	PCUT_ASSERT_EQUALS('\0', a);

	a = ui_accel_get("~~");
	PCUT_ASSERT_EQUALS('\0', a);

	a = ui_accel_get("~~Hello~~");
	PCUT_ASSERT_EQUALS('\0', a);

	a = ui_accel_get("~H~ello");
	PCUT_ASSERT_EQUALS('h', a);

	a = ui_accel_get("H~e~llo");
	PCUT_ASSERT_EQUALS('e', a);
}

PCUT_EXPORT(accel);
