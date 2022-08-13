/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tmpfs
 * @{
 */

/**
 * @file	tmpfs.c
 * @brief	File system driver for in-memory file system.
 *
 * Every instance of tmpfs exists purely in memory and has neither a disk layout
 * nor any permanent storage (e.g. disk blocks). With each system reboot, data
 * stored in a tmpfs file system is lost.
 */

#include "tmpfs.h"
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <task.h>
#include <libfs.h>
#include <str.h>
#include "../../vfs/vfs.h"

#define NAME  "tmpfs"

vfs_info_t tmpfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char **argv)
{
	printf("%s: HelenOS TMPFS file system server\n", NAME);

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			tmpfs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf("%s: Unrecognized parameters", NAME);
			return -1;
		}
	}

	if (!tmpfs_init()) {
		printf("%s: Failed to initialize TMPFS\n", NAME);
		return -1;
	}

	errno_t rc;
	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf("%s: Unable to connect to VFS: %s\n", NAME, str_error(rc));
		return -1;
	}

	rc = fs_register(vfs_sess, &tmpfs_vfs_info, &tmpfs_ops,
	    &tmpfs_libfs_ops);
	if (rc != EOK) {
		printf("%s: Failed to register file system: %s\n", NAME,
		    str_error(rc));
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
