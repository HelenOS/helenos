/*
 * SPDX-FileCopyrightText: 2012-2018 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>

#ifdef __helenos__
#include <fibril.h>
#else
#if defined(__unix) || defined(__APPLE__)
#include <unistd.h>
#endif
#if defined(__WIN64) || defined(__WIN32) || defined(_WIN32)
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include "tested.h"

static void my_sleep(int sec) {
#ifdef __helenos__
	fibril_sleep(sec);
#else
#if defined(__unix) || defined(__APPLE__)
	sleep(sec);
#endif
#if defined(__WIN64) || defined(__WIN32) || defined(_WIN32)
	Sleep(1000 * sec);
#endif
#endif
}

PCUT_INIT

PCUT_TEST(shall_time_out) {
	printf("Text before sleeping.\n");
	my_sleep(PCUT_DEFAULT_TEST_TIMEOUT * 5);
	printf("Text after the sleep.\n");
}

PCUT_TEST(custom_time_out,
		PCUT_TEST_SET_TIMEOUT(PCUT_DEFAULT_TEST_TIMEOUT * 3)) {
	printf("Text before sleeping.\n");
	my_sleep(PCUT_DEFAULT_TEST_TIMEOUT * 2);
	printf("Text after the sleep.\n");
}

PCUT_MAIN()
