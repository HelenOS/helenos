/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2010 Jiri Kavalik
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
	
	async_sess_t *vfs_sess = service_connect_blocking(SERVICE_VFS,
	    INTERFACE_VFS_DRIVER, 0);
	if (!vfs_sess) {
		printf("%s: Unable to connect to VFS\n", NAME);
		return -1;
	}
	
	errno_t rc = fs_register(vfs_sess, &cdfs_vfs_info, &cdfs_ops,
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
