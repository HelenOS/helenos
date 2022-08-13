/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <task.h>
#include <pcut/pcut.h>
#include <perm.h>

PCUT_INIT;

PCUT_TEST_SUITE(perm);

PCUT_TEST(revoke)
{
	errno_t rc = perm_revoke(task_get_id(), 0xf); // XXX Need PERM_xxx
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_EXPORT(perm);
