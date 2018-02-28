/*
 * Copyright (c) 2016 Jakub Jermar
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
#include <vfs/vfs.h>
#include <stdlib.h>
#include <as.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include "../tester.h"

#define TEST_FILE	"/tmp/testfile"

const char text[] = "Hello world!";

int fd;

static void *create_paged_area(size_t size)
{
	size_t nwr;
	errno_t rc;

	TPRINTF("Creating temporary file...\n");

	rc = vfs_lookup_open(TEST_FILE, WALK_REGULAR | WALK_MAY_CREATE,
	    MODE_READ | MODE_WRITE, &fd);
	if (rc != EOK)
		return NULL;
	(void) vfs_unlink_path(TEST_FILE);

	rc = vfs_write(fd, (aoff64_t []) {0}, text, sizeof(text), &nwr);
	if (rc != EOK) {
		vfs_put(fd);
		return NULL;
	}

	async_sess_t *vfs_pager_sess;

	TPRINTF("Connecting to VFS pager...\n");

	vfs_pager_sess = service_connect_blocking(SERVICE_VFS, INTERFACE_PAGER, 0);

	if (!vfs_pager_sess) {
		vfs_put(fd);
		return NULL;
	}

	TPRINTF("Creating AS area...\n");

	void *result = async_as_area_create(AS_AREA_ANY, size,
	    AS_AREA_READ | AS_AREA_CACHEABLE, vfs_pager_sess, fd, 0, 0);
	if (result == AS_MAP_FAILED) {
		vfs_put(fd);
		return NULL;
	}

	return result;
}

static void touch_area(void *area, size_t size)
{
	TPRINTF("Touching (faulting-in) AS area...\n");

	volatile char *ptr = (char *) area;

	char ch;
	while ((ch = *ptr++))
		putchar(ch);
}

const char *test_pager1(void)
{
	size_t buffer_len = PAGE_SIZE;
	void *buffer = create_paged_area(buffer_len);
	if (!buffer)
		return "Cannot allocate memory";

	touch_area(buffer, buffer_len);

	as_area_destroy(buffer);
	vfs_put(fd);

	return NULL;
}
