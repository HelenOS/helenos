/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
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
	    INTERFACE_VFS_DRIVER, 0);

	if (!vfs_sess) {
		printf(NAME ": failed to connect to VFS\n");
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

