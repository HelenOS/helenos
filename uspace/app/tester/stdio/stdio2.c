/*
 * Copyright (c) 2008 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
	const char *file_name = "/test";

	TPRINTF("Open file \"%s\" for writing...", file_name);
	errno = 0;
	file = fopen(file_name, "wt");
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
