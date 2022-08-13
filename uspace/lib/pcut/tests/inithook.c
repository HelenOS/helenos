/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

static int init_counter = 0;

static void init_hook(void) {
	init_counter++;
}

PCUT_TEST_BEFORE {
	PCUT_ASSERT_INT_EQUALS(1, init_counter);
	init_counter++;
}

PCUT_TEST(check_init_counter) {
	PCUT_ASSERT_INT_EQUALS(2, init_counter);
}

PCUT_TEST(check_init_counter_2) {
	PCUT_ASSERT_INT_EQUALS(2, init_counter);
}


PCUT_CUSTOM_MAIN(
	PCUT_MAIN_SET_INIT_HOOK(init_hook)
)

