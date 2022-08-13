/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <io/table.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(table);

PCUT_TEST(smoke)
{
	table_t *table;
	errno_t rc;

	rc = table_create(&table);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	table_header_row(table);
	rc = table_printf(table, "A\t" "B\t" "C\t" "D\n");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = table_printf(table, "1\t" "2\t" "3\t" "4\n");
	rc = table_printf(table, "i\t" "ii\t" "iii\t" "iv\n");
	table_destroy(table);
}

PCUT_EXPORT(table);
