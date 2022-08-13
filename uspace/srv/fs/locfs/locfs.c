/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup locfs
 * @{
 */

/**
 * @file locfs.c
 * @brief Location-service file system.
 *
 * Every service registered with location service is represented as a file in this
 * file system.
 */

#include <stdio.h>
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <task.h>
#include <libfs.h>
#include <str.h>
#include "locfs.h"
#include "locfs_ops.h"

#define NAME  "locfs"

static vfs_info_t locfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Device Filesystem\n", NAME);

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			locfs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf(NAME " Unrecognized parameters");
			return -1;
		}
	}

	if (!locfs_init()) {
		printf("%s: failed to initialize locfs\n", NAME);
		return -1;
	}

	errno_t rc;
	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf("%s: Unable to connect to VFS: %s\n", NAME, str_error(rc));
		return -1;
	}

	rc = fs_register(vfs_sess, &locfs_vfs_info, &locfs_ops,
	    &locfs_libfs_ops);
	if (rc != EOK) {
		printf("%s: Failed to register file system: %s\n", NAME, str_error(rc));
		return rc;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
