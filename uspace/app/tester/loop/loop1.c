/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "../tester.h"

const char *test_loop1(void)
{
	TPRINTF("Looping...");
	while (true)
		;
	TPRINTF("\n");

	return "Survived endless loop";
}
