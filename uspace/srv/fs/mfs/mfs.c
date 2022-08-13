/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2011 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup mfs
 * @{
 */

/**
 * @file	mfs.c
 * @brief	Minix file system driver for HelenOS.
 */

#include <ipc/services.h>
#include <stdlib.h>
#include <str.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <task.h>
#include <stdio.h>
#include <libfs.h>
#include "mfs.h"

vfs_info_t mfs_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char **argv)
{
	errno_t rc;

	printf(NAME ": HelenOS Minix file system server\n");

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			mfs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf(NAME " Unrecognized parameters");
			rc = EINVAL;
			goto err;
		}
	}

	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf(NAME ": failed to connect to VFS: %s\n", str_error(rc));
		rc = errno;
		goto err;
	}

	rc = mfs_global_init();
	if (rc != EOK) {
		printf(NAME ": Failed global initialization\n");
		goto err;
	}

	rc = fs_register(vfs_sess, &mfs_vfs_info, &mfs_ops, &mfs_libfs_ops);
	if (rc != EOK)
		goto err;

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();
	/* not reached */
	return 0;

err:
	printf(NAME ": Failed to register file system: %s\n", str_error(rc));
	return rc;
}

/**
 * @}
 */
