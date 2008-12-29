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
#include <errno.h>
#include "../tester.h"

#define BUF_SIZE 32
static char buf[BUF_SIZE + 1];

char * test_stdio1(bool quiet)
{
	FILE *f;
	char *file_name = "/readme";
	size_t n;
	int c;

	printf("Open file '%s'\n", file_name);
	errno = 0;
	f = fopen(file_name, "rt");

	if (f == NULL) printf("errno = %d\n", errno);

	if (f == NULL)
		return "Failed opening file.";

	n = fread(buf, 1, BUF_SIZE, f);
	if (ferror(f)) {
		fclose(f);
		return "Failed reading file.";
	}

	printf("Read %d bytes.\n", n);

	buf[n] = '\0';
	printf("Read string '%s'.\n", buf);

	printf("Seek to beginning.\n");
	if (fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return "Failed seeking.";
	}

	printf("Read using fgetc().\n");
	while (true) {
		c = fgetc(f);
		if (c == EOF) break;

		printf("'%c'", c);
	}

	printf("[EOF]\n");
	printf("Closing.\n");

	if (fclose(f) != 0)
		return "Failed closing.";

	return NULL;
}
