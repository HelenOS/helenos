/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ext4fs
 * @{
 */
/**
 * @file  ext4fs.c
 * @brief Ext4 file system driver for HelenOS.
 */

#include <async.h>
#include <errno.h>
#include <libfs.h>
#include <ns.h>
#include <stdio.h>
#include <task.h>
#include <ipc/services.h>
#include <str.h>
#include <str_error.h>
#include "ext4/ops.h"
#include "../../vfs/vfs.h"

#define NAME  "ext4fs"

vfs_info_t ext4fs_vfs_info = {
	.name = NAME,
	.instance = 0
};

int main(int argc, char **argv)
{
	printf("%s: HelenOS ext4 file system server\n", NAME);

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			ext4fs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf("%s: Unrecognized parameters\n", NAME);
			return 1;
		}
	}

	errno_t rc;
	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf("%s: Failed to connect to VFS: %s\n", NAME, str_error(rc));
		return 2;
	}

	rc = ext4_global_init();
	if (rc != EOK) {
		printf("%s: Global initialization failed: %s\n", NAME,
		    str_error(rc));
		return rc;
	}

	rc = fs_register(vfs_sess, &ext4fs_vfs_info, &ext4_ops,
	    &ext4_libfs_ops);
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
