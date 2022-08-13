/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <str_error.h>
#include <errno.h>
#include "../tester.h"

const char *test_stdio2(void)
{
	FILE *file;
	const char *file_name = "/tmp/test";

	TPRINTF("Open file \"%s\" for writing...", file_name);
	errno = 0;
	file = fopen(file_name, "wtx");
	if (file == NULL) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed opening file";
	} else
		TPRINTF("OK\n");

	TPRINTF("Write to file...");
	fprintf(file, "integer: %u, string: \"%s\"", 42, "Hello!");
	TPRINTF("OK\n");

	TPRINTF("Close...");
	if (fclose(file) != 0) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed closing file";
	} else
		TPRINTF("OK\n");

	TPRINTF("Open file \"%s\" for reading...", file_name);
	file = fopen(file_name, "rt");
	if (file == NULL) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed opening file";
	} else
		TPRINTF("OK\n");

	TPRINTF("File contains:\n");
	while (true) {
		int c = fgetc(file);
		if (c == EOF)
			break;
		TPRINTF("%c", c);
	}

	TPRINTF("\nClose...");
	if (fclose(file) != 0) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed closing file";
	} else
		TPRINTF("OK\n");

	return NULL;
}
