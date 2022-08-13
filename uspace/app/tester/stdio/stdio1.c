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

#define BUF_SIZE  32

static char buf[BUF_SIZE + 1];

const char *test_stdio1(void)
{
	FILE *file;
	const char *file_name = "/demo.txt";

	TPRINTF("Open file \"%s\"...", file_name);
	errno = 0;
	file = fopen(file_name, "rt");
	if (file == NULL) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed opening file";
	} else
		TPRINTF("OK\n");

	TPRINTF("Read file...");
	size_t cnt = fread(buf, 1, BUF_SIZE, file);
	if (ferror(file)) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		fclose(file);
		return "Failed reading file";
	} else
		TPRINTF("OK\n");

	buf[cnt] = '\0';
	TPRINTF("Read %zu bytes, string \"%s\"\n", cnt, buf);

	TPRINTF("Seek to beginning...");
	if (fseek(file, 0, SEEK_SET) != 0) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		fclose(file);
		return "Failed seeking in file";
	} else
		TPRINTF("OK\n");

	TPRINTF("Read using fgetc()...");
	while (true) {
		int c = fgetc(file);
		if (c == EOF)
			break;

		TPRINTF(".");
	}
	TPRINTF("[EOF]\n");

	TPRINTF("Close...");
	if (fclose(file) != 0) {
		TPRINTF("errno = %s\n", str_error_name(errno));
		return "Failed closing file";
	} else
		TPRINTF("OK\n");

	return NULL;
}
