/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

	rc = vfs_write(fd, (aoff64_t []) { 0 }, text, sizeof(text), &nwr);
	if (rc != EOK) {
		vfs_put(fd);
		return NULL;
	}

	async_sess_t *vfs_pager_sess;

	TPRINTF("Connecting to VFS pager...\n");

	vfs_pager_sess = service_connect_blocking(SERVICE_VFS, INTERFACE_PAGER,
	    0, NULL);

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
