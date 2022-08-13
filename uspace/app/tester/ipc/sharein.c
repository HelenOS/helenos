/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
