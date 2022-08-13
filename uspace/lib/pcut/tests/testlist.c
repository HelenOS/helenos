/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

static char *argv_patched[] = {
	NULL, /* Will be patched at run-time. */
	(char *) "-l",
	NULL
};

static void pre_init_hook(int *argc, char **argv[]) {
	argv_patched[0] = (*argv)[0];
	*argc = 2;
	*argv = argv_patched;
}

PCUT_TEST(unreachable) {
	PCUT_ASSERT_TRUE(0 && "unreachable code");
}


PCUT_CUSTOM_MAIN(
	PCUT_MAIN_SET_PREINIT_HOOK(pre_init_hook)
)

