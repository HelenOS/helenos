/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <errno.h>
#include <str_error.h>
#include <ipc/services.h>
#include <io/chardev.h>
#include <loc.h>
#include "../tester.h"

#define SMALL_BUFFER_SIZE 64
#define LARGE_BUFFER_SIZE (DATA_XFER_LIMIT * 4)

static char small_buffer[SMALL_BUFFER_SIZE];
static char large_buffer[LARGE_BUFFER_SIZE];

/** Test device that always performs small transfers. */
static const char *test_chardev1_smallx(void)
{
	chardev_t *chardev;
	service_id_t sid;
	async_sess_t *sess;
	size_t nbytes;
	errno_t rc;

	TPRINTF("Test small transfer character device operations\n");

	rc = loc_service_get_id(SERVICE_NAME_CHARDEV_TEST_SMALLX, &sid, 0);
	if (rc != EOK) {
		return "Failed resolving test device "
		    SERVICE_NAME_CHARDEV_TEST_SMALLX;
	}

	sess = loc_service_connect(sid, INTERFACE_DDF, 0);
	if (sess == NULL)
		return "Failed connecting test device";

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		async_hangup(sess);
		return "Failed opening test device";
	}

	rc = chardev_write(chardev, small_buffer, SMALL_BUFFER_SIZE, &nbytes);
	if (rc != EOK) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed sending data";
	}

	TPRINTF("Sent %zu bytes\n", nbytes);

	rc = chardev_read(chardev, small_buffer, SMALL_BUFFER_SIZE, &nbytes,
	    chardev_f_none);
	if (rc != EOK) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed receiving data";
	}

	TPRINTF("Received %zu bytes\n", nbytes);

	chardev_close(chardev);
	async_hangup(sess);

	TPRINTF("Done\n");
	return NULL;
}

/** Test device that always performs large transfers. */
static const char *test_chardev1_largex(void)
{
	chardev_t *chardev;
	service_id_t sid;
	async_sess_t *sess;
	size_t nbytes;
	errno_t rc;

	TPRINTF("Test large transfer character device operations\n");

	rc = loc_service_get_id(SERVICE_NAME_CHARDEV_TEST_LARGEX, &sid, 0);
	if (rc != EOK) {
		return "Failed resolving test device "
		    SERVICE_NAME_CHARDEV_TEST_LARGEX;
	}

	sess = loc_service_connect(sid, INTERFACE_DDF, 0);
	if (sess == NULL)
		return "Failed connecting test device";

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		async_hangup(sess);
		return "Failed opening test device";
	}

	rc = chardev_write(chardev, large_buffer, LARGE_BUFFER_SIZE, &nbytes);
	if (rc != EOK) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed sending data";
	}

	TPRINTF("Sent %zu bytes\n", nbytes);

	rc = chardev_read(chardev, large_buffer, LARGE_BUFFER_SIZE, &nbytes,
	    chardev_f_none);
	if (rc != EOK) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed receiving data";
	}

	TPRINTF("Received %zu bytes\n", nbytes);

	chardev_close(chardev);
	async_hangup(sess);

	TPRINTF("Done\n");
	return NULL;
}

/** Test device where all transfers return partial success. */
static const char *test_chardev1_partialx(void)
{
	chardev_t *chardev;
	service_id_t sid;
	async_sess_t *sess;
	size_t nbytes;
	errno_t rc;

	TPRINTF("Test partially-successful character device operations\n");

	rc = loc_service_get_id(SERVICE_NAME_CHARDEV_TEST_PARTIALX, &sid, 0);
	if (rc != EOK) {
		return "Failed resolving test device "
		    SERVICE_NAME_CHARDEV_TEST_SMALLX;
	}

	sess = loc_service_connect(sid, INTERFACE_DDF, 0);
	if (sess == NULL)
		return "Failed connecting test device";

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		async_hangup(sess);
		return "Failed opening test device";
	}

	rc = chardev_write(chardev, small_buffer, SMALL_BUFFER_SIZE, &nbytes);
	if (rc != EIO || nbytes != 1) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed sending data";
	}

	TPRINTF("Sent %zu bytes and got rc = %s (expected)\n", nbytes,
	    str_error_name(rc));

	rc = chardev_read(chardev, small_buffer, SMALL_BUFFER_SIZE, &nbytes,
	    chardev_f_none);
	if (rc != EIO || nbytes != 1) {
		chardev_close(chardev);
		async_hangup(sess);
		return "Failed receiving data";
	}

	TPRINTF("Received %zu bytes and got rc = %s (expected)\n", nbytes,
	    str_error_name(rc));

	chardev_close(chardev);
	async_hangup(sess);

	TPRINTF("Done\n");
	return NULL;
}

const char *test_chardev1(void)
{
	const char *s;

	s = test_chardev1_smallx();
	if (s != NULL)
		return s;

	s = test_chardev1_largex();
	if (s != NULL)
		return s;

	return test_chardev1_partialx();
}
