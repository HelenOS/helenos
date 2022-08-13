/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 * SPDX-FileCopyrightText: 2012 Julia Medvedeva
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udf
 * @{
 */
/**
 * @file udf.c
 * @brief UDF 1.02 file system driver for HelenOS.
 */

#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <task.h>
#include <libfs.h>
#include <str.h>
#include <io/log.h>
#include "../../vfs/vfs.h"
#include "udf.h"
#include "udf_idx.h"

#define NAME  "udf"

vfs_info_t udf_vfs_info = {
	.name = NAME,
	.concurrent_read_write = false,
	.write_retains_size = false,
	.instance = 0,
};

int main(int argc, char *argv[])
{
	log_init(NAME);
	log_msg(LOG_DEFAULT, LVL_NOTE, "HelenOS UDF 1.02 file system server");

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			udf_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			log_msg(LOG_DEFAULT, LVL_FATAL, "Unrecognized parameters");
			return 1;
		}
	}

	errno_t rc;
	async_sess_t *vfs_sess =
	    service_connect_blocking(SERVICE_VFS, INTERFACE_VFS_DRIVER, 0, &rc);
	if (!vfs_sess) {
		log_msg(LOG_DEFAULT, LVL_FATAL, "Failed to connect to VFS: %s",
		    str_error(rc));
		return 2;
	}

	rc = fs_register(vfs_sess, &udf_vfs_info, &udf_ops,
	    &udf_libfs_ops);
	if (rc != EOK)
		goto err;

	rc = udf_idx_init();
	if (rc != EOK)
		goto err;

	log_msg(LOG_DEFAULT, LVL_NOTE, "Accepting connections");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;

err:
	log_msg(LOG_DEFAULT, LVL_FATAL, "Failed to register file system: %s", str_error(rc));
	return rc;
}

/**
 * @}
 */
