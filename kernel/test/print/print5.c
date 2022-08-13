/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This test tests several features of the HelenOS
 * printf() implementation which go beyond the POSIX
 * specification and GNU printf() behaviour.
 *
 * Therefore we disable printf() argument checking by
 * the GCC compiler in this source file to avoid false
 * positives.
 *
 */
#pragma GCC diagnostic ignored "-Wformat"

#include <stddef.h>
#include <test.h>

const char *test_print5(void)
{
	TPRINTF("Testing printf(\"%%s\", NULL):\n");
	TPRINTF("Expected output: \"(NULL)\"\n");
	TPRINTF("Real output:     \"%s\"\n\n", (char *) NULL);

	TPRINTF("Testing printf(\"%%c %%3.2c %%-3.2c %%2.3c %%-2.3c\", 'a', 'b', 'c', 'd', 'e'):\n");
	TPRINTF("Expected output: [a] [  b] [c  ] [ d] [e ]\n");
	TPRINTF("Real output:     [%c] [%3.2c] [%-3.2c] [%2.3c] [%-2.3c]\n\n", 'a', 'b', 'c', 'd', 'e');

	return NULL;
}
