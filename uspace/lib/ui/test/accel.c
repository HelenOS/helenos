/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
