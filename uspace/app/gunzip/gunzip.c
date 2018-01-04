/*
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup gunzip
 * @{
 */
/** @file
 */

#include <errno.h>
#include <gzip.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	errno_t rc;
	void *data, *ddata;
	size_t size, dsize;
	size_t nread, nwr;
	long len;
	FILE *f, *wf;

	if (argc != 3) {
		printf("syntax: gunzip <src.gz> <dest>\n");
		return 1;
	}

	f = fopen(argv[1], "rb");
	if (f == NULL) {
		printf("Error opening '%s'\n", argv[1]);
		return 1;
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		printf("Error determining size of '%s'\n", argv[1]);
		fclose(f);
		return 1;
	}

	len = ftell(f);
	if (len < 0) {
		printf("Error determining size of '%s'\n", argv[1]);
		fclose(f);
		return 1;
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		printf ("Error rewinding '%s'\n", argv[1]);
		fclose(f);
		return 1;
	}

	data = malloc(len);
	if (data == NULL) {
		printf("Error allocating %ld bytes.\n", len);
		fclose(f);
		return 1;
	}

	nread = fread(data, 1, len, f);
	if (nread != (size_t)len) {
		printf("Error reading '%s'\n", argv[1]);
		fclose(f);
		return 1;
	}

	fclose(f);

	size = (size_t) len;

	rc = gzip_expand(data, size, &ddata, &dsize);
	if (rc != EOK) {
		printf("Error decompressing data.\n");
		return 1;
	}

	wf = fopen(argv[2], "wb");
	if (wf == NULL) {
		printf("Error creating file '%s'\n", argv[2]);
		return 1;
	}

	nwr = fwrite(ddata, 1, dsize, wf);
	if (nwr != dsize) {
		printf("Error writing '%s'\n", argv[2]);
		fclose(wf);
		return 1;
	}

	if (fclose(wf) != 0) {
		printf("Error writing '%s'\n", argv[2]);
		return 1;
	}

	return 0;
}

/** @}
 */
