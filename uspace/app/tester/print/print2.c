/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stddef.h>
#include "../tester.h"

const char *test_print2(void)
{
	TPRINTF("Testing printf(\"%%c\", 'a'):\n");
	TPRINTF("Expected output: [a]\n");
	TPRINTF("Real output:     [%c]\n\n", 'a');

	TPRINTF("Testing printf(\"%%d %%3.2d %%-3.2d %%2.3d %%-2.3d\", 1, 2, 3, 4, 5):\n");
	TPRINTF("Expected output: [1] [ 02] [03 ] [004] [005]\n");
	TPRINTF("Real output:     [%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]\n\n", 1, 2, 3, 4, 5);

	TPRINTF("Testing printf(\"%%d %%3.2d %%-3.2d %%2.3d %%-2.3d\", -1, -2, -3, -4, -5):\n");
	TPRINTF("Expected output: [-1] [-02] [-03] [-004] [-005]\n");
	TPRINTF("Real output:     [%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]\n\n", -1, -2, -3, -4, -5);

	TPRINTF("Testing printf(\"%%lld %%3.2lld %%-3.2lld %%2.3lld %%-2.3lld\", (long long) -1, (long long) -2, (long long) -3, (long long) -4, (long long) -5):\n");
	TPRINTF("Expected output: [-1] [-02] [-03] [-004] [-005]\n");
	TPRINTF("Real output:     [%lld] [%3.2lld] [%-3.2lld] [%2.3lld] [%-2.3lld]\n\n", (long long) -1, (long long) -2, (long long) -3, (long long) -4, (long long) -5);

	TPRINTF("Testing printf(\"%%#x %%5.3#x %%-5.3#x %%3.5#x %%-3.5#x\", 17, 18, 19, 20, 21):\n");
	TPRINTF("Expected output: [0x11] [0x012] [0x013] [0x00014] [0x00015]\n");
	TPRINTF("Real output:     [%#x] [%#5.3x] [%#-5.3x] [%#3.5x] [%#-3.5x]\n\n", 17, 18, 19, 20, 21);

	char ch[12];
	ptrdiff_t d, neg_d;

	d = &ch[0] - &ch[12];
	neg_d = (unsigned)(-d);
	TPRINTF("Testing printf(\"%%td %%tu %%tx %%ti %%to\", d, neg_d, neg_d, d, neg_d):\n");
	TPRINTF("Expected output: [-12] [12] [c] [-12] [14]\n");
	TPRINTF("Real output:     [%td] [%tu] [%tx] [%ti] [%to]\n\n", d, neg_d, neg_d, d, neg_d);

	return NULL;
}
