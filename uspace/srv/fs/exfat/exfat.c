/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup exfat
 * @{
 */

/**
 * @file	exfat.c
 * @brief	exFAT file system driver for HelenOS.
 */

#include "exfat.h"
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <task.h>
#include <stdio.h>
#include <libfs.h>
#include <str.h>
#include "../../vfs/vfs.h"

#define NAME	"exfat"

vfs_info_t exfat_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS exFAT file system server\n");

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			exfat_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf(NAME " Unrecognized parameters");
			return -1;
		}
	}

	errno_t rc = exfat_idx_init();
	if (rc != EOK)
		goto err;

	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		printf(NAME ": failed to connect to VFS: %s\n", str_error(rc));
		return -1;
	}

	rc = fs_register(vfs_sess, &exfat_vfs_info, &exfat_ops, &exfat_libfs_ops);
	if (rc != EOK) {
		exfat_idx_fini();
		goto err;
	}

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
