/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../tester.h"
#include <stdio.h>

typedef int __attribute__((may_alias)) aliasing_int;

const char *test_fault2(void)
{
	volatile long long var = 0;
	volatile int var1 = *((aliasing_int *) (((char *) (&var)) + 1));
	printf("Read %d\n", var1);

	return "Survived unaligned read";
}
