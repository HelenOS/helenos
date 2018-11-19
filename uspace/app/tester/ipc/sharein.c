/*
 * Copyright (c) 2018 Jiri Svoboda
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ipc_test.h>
#include "../tester.h"

const char *test_sharein(void)
{
	ipc_test_t *test = NULL;
	const void *ro_ptr;
	size_t ro_size;
	void *rw_ptr;
	size_t rw_size;
	errno_t rc;

	rc = ipc_test_create(&test);
	if (rc != EOK)
		return "Error contacting IPC test service.";

	rc = ipc_test_get_ro_area_size(test, &ro_size);
	if (rc != EOK)
		return "Error getting read-only area size.";

	rc = ipc_test_share_in_ro(test, ro_size, &ro_ptr);
	if (rc != EOK)
		return "Error sharing in area.";

	TPRINTF("Successfully shared in read-only area.\n");
	TPRINTF("Byte from shared read-only area: 0x%02x\n",
	    *(const uint8_t *)ro_ptr);

	as_area_destroy((void *)ro_ptr);

	rc = ipc_test_get_rw_area_size(test, &rw_size);
	if (rc != EOK)
		return "Error getting read-write area size.";

	rc = ipc_test_share_in_rw(test, rw_size, &rw_ptr);
	if (rc != EOK)
		return "Error sharing in area.";

	TPRINTF("Successfully shared in read-write area.\n");
	TPRINTF("Byte from shared read-write area: 0x%02x\n",
	    *(uint8_t *)rw_ptr);

	ipc_test_destroy(test);
	return NULL;
}
