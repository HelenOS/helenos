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

#define BUF_SIZE  32

static char buf[BUF_SIZE + 1];

const char *test_stdio1(void)
{
	FILE *file;
	const char *file_name = "/textdemo";

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
