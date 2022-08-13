/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <stdlib.h>

PCUT_INIT

PCUT_TEST(access_null_pointer) {
	abort();
}

PCUT_MAIN()
