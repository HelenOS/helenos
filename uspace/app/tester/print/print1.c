/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stddef.h>
#include "../tester.h"

const char *test_print1(void)
{
	TPRINTF("Testing printf(\"%%*.*s\", 5, 3, \"text\"):\n");
	TPRINTF("Expected output: \"  tex\"\n");
	TPRINTF("Real output:     \"%*.*s\"\n\n", 5, 3, "text");

	TPRINTF("Testing printf(\"%%10.8s\", \"very long text\"):\n");
	TPRINTF("Expected output: \"  very lon\"\n");
	TPRINTF("Real output:     \"%10.8s\"\n\n", "very long text");

	TPRINTF("Testing printf(\"%%8.10s\", \"text\"):\n");
	TPRINTF("Expected output: \"    text\"\n");
	TPRINTF("Real output:     \"%8.10s\"\n\n", "text");

	TPRINTF("Testing printf(\"%%8.10s\", \"very long text\"):\n");
	TPRINTF("Expected output: \"very long \"\n");
	TPRINTF("Real output:     \"%8.10s\"\n\n", "very long text");

	TPRINTF("Testing printf(\"%%-*.*s\", 7, 7, \"text\"):\n");
	TPRINTF("Expected output: \"text   \"\n");
	TPRINTF("Real output:     \"%-*.*s\"\n\n", 7, 7, "text");

	return NULL;
}
