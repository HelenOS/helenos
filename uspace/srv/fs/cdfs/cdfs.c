/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2010 Jiri Kavalik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup cdfs
 * @{
 */

/**
 * @file cdfs.c
 * @brief File system driver for ISO9660 file system.
 */

#include <ipc/services.h>
#include <ns.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <libfs.h>
#include <str.h>
#include "cdfs.h"
#include "cdfs_ops.h"

#define NAME  "cdfs"

static vfs_info_t cdfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char **argv)
{
	printf("%s: HelenOS cdfs file system server\n", NAME);

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			cdfs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf(NAME " Unrecognized parameters");
			return -1;
		}
	}

	if (!cdfs_init()) {
		printf("%s: failed to initialize cdfs\n", NAME);
		return -1;
	}

	errno_t rc;
	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf("%s: Unable to connect to VFS: %s\n", NAME, str_error(rc));
		return -1;
	}

	rc = fs_register(vfs_sess, &cdfs_vfs_info, &cdfs_ops,
	    &cdfs_libfs_ops);
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
