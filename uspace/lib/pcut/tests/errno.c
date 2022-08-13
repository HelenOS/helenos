/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <errno.h>
#include "tested.h"

#ifndef EOK
#define EOK 0
#endif

PCUT_INIT

PCUT_TEST(errno_value) {
	int value = EOK;
	PCUT_ASSERT_ERRNO_VAL(EOK, value);
	value = ENOENT;
	PCUT_ASSERT_ERRNO_VAL(ENOENT, value);

	/* This fails. */
	PCUT_ASSERT_ERRNO_VAL(EOK, value);
}

PCUT_TEST(errno_variable) {
	errno = ENOENT;
	PCUT_ASSERT_ERRNO(ENOENT);

	/* This fails. */
	PCUT_ASSERT_ERRNO(EOK);
}

PCUT_MAIN()
