/*
 * Copyright (c) 2023 Jiri Svoboda
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

#include <as.h>
#include <errno.h>
#include <mem.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ipc_test.h>
#include "../tester.h"

enum {
	rw_buf_size = 1024
};

static uint8_t rw_buf[rw_buf_size];

const char *test_readwrite(void)
{
	ipc_test_t *test = NULL;
	size_t i;
	errno_t rc;

	rc = ipc_test_create(&test);
	if (rc != EOK)
		return "Error contacting IPC test service.";

	rc = ipc_test_set_rw_buf_size(test, rw_buf_size);
	if (rc != EOK)
		return "Error getting read-only area size.";

	/*
	 * Write all zeroes to remote buffer
	 */
	memset(rw_buf, 0x00, rw_buf_size);
	rc = ipc_test_write(test, rw_buf, rw_buf_size);
	if (rc != EOK)
		return "Error writing remote buffer.";

	TPRINTF("Successfully wrote zeroes to remote buffer.\n");

	/*
	 * Read back contents of remote buffer and verify
	 */

	/*
	 * Make sure the contents of local buffer are different from what
	 * we expect to read.
	 */
	memset(rw_buf, 0xff, rw_buf_size);
	rc = ipc_test_read(test, rw_buf, rw_buf_size);
	if (rc != EOK)
		return "Error reading remote buffer.";

	TPRINTF("Successfully read back remote buffer.\n");

	/* Now verify what we have read */
	for (i = 0; i < rw_buf_size; i++) {
		if (rw_buf[i] != 0x00)
			return "Failed verification of read data.";
	}

	TPRINTF("Read data succeeded verification.\n");

	/*
	 * Write all binary ones to remote buffer
	 */
	memset(rw_buf, 0xff, rw_buf_size);
	rc = ipc_test_write(test, rw_buf, rw_buf_size);
	if (rc != EOK)
		return "Error writing remote buffer.";

	TPRINTF("Successfully wrote binary ones to remote buffer.\n");

	/*
	 * Read back contents of remote buffer and verify
	 */

	/*
	 * Make sure the contents of local buffer are different from what
	 * we expect to read.
	 */
	memset(rw_buf, 0x00, rw_buf_size);
	rc = ipc_test_read(test, rw_buf, rw_buf_size);
	if (rc != EOK)
		return "Error reading remote buffer.";

	TPRINTF("Successfully read back remote buffer.\n");

	/* Now verify what we have read */
	for (i = 0; i < rw_buf_size; i++) {
		if (rw_buf[i] != 0xff)
			return "Failed verification of read data.";
	}

	TPRINTF("Read data succeeded verification.\n");

	ipc_test_destroy(test);
	return NULL;
}
