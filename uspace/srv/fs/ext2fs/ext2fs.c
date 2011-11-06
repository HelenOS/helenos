/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2011 Martin Sucha
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
 * @file	ext2.c
 * @brief	EXT2 file system driver for HelenOS.
 */

#include "ext2fs.h"
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdio.h>
#include <libfs.h>
#include "../../vfs/vfs.h"

#define NAME	"ext2fs"

vfs_info_t ext2fs_vfs_info = {
	.name = NAME,
	.instance = 0,
};

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS EXT2 file system server\n");

	if (argc == 3) {
		if (!str_cmp(argv[1], "--instance"))
			ext2fs_vfs_info.instance = strtol(argv[2], NULL, 10);
		else {
			printf(NAME " Unrecognized parameters");
			return -1;
		}
	}
	
	async_sess_t *vfs_sess = service_connect_blocking(EXCHANGE_SERIALIZE,
	    SERVICE_VFS, 0, 0);
	if (!vfs_sess) {
		printf(NAME ": failed to connect to VFS\n");
		return -1;
	}

	int rc = ext2fs_global_init();
	if (rc != EOK) {
		printf(NAME ": Failed global initialization\n");
		return 1;
	}	
		
	rc = fs_register(vfs_sess, &ext2fs_vfs_info, &ext2fs_ops,
	    &ext2fs_libfs_ops);
	if (rc != EOK) {
		fprintf(stdout, NAME ": Failed to register fs (%d)\n", rc);
		return 1;
	}
	
	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();
	/* not reached */
	return 0;
}

/**
 * @}
 */ 
