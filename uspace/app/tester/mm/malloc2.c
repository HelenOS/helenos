/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "../tester.h"

const char *test_malloc2(void)
{
	int cnt = 0;
	char *p;

	TPRINTF("Provoking the kernel into overcommitting memory to us...\n");
	while ((p = malloc(1024 * 1024))) {
		TPRINTF("%dM ", ++cnt);
		*p = 'A';
	}
	TPRINTF("\nWas refused more memory as expected.\n");

	return NULL;
}
