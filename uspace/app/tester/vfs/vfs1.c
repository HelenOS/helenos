/*
 * Copyright (c) 2007 Jakub Jermar
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

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <async.h>
#include "../../../srv/vfs/vfs.h" 
#include "../tester.h"

#define TMPFS_DEVHANDLE	999

char fs_name[] = "tmpfs";
char mp[] = "/";

char *test_vfs1(bool quiet)
{
	/* 1. connect to VFS */
	int vfs_phone;
	vfs_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VFS, 0, 0);
	if (vfs_phone < EOK) {
		return "Could not connect to VFS.\n";
	}
	
	/* 2. mount TMPFS as / */
	ipcarg_t rc;
	aid_t req;
	req = async_send_1(vfs_phone, VFS_MOUNT, TMPFS_DEVHANDLE, NULL);
	if (ipc_data_send(vfs_phone, fs_name, strlen(fs_name)) != EOK) {
		async_wait_for(req, &rc);
		return "Could not send fs_name to VFS.\n";
	}
	if (ipc_data_send(vfs_phone, mp, strlen(mp)) != EOK) {
		async_wait_for(req, &rc);
		return "Could not send mp to VFS.\n";
	}
	async_wait_for(req, &rc);
	if (rc != EOK) {
		return "Mount failed.\n";
	}
	
	/* 3. open some files */
	char *path = "/dir2/file2";
	ipc_call_t answer;
	req = async_send_2(vfs_phone, VFS_OPEN, 0, 0, &answer);
	if (ipc_data_send(vfs_phone, path, strlen(path)) != EOK) {
		async_wait_for(req, &rc);
		return "Could not send path to VFS.\n";
	}
	async_wait_for(req, &rc);
	if (rc != EOK) {
		return "Open failed.\n";
	}

	if (!quiet) {
		printf("Opened %s handle=%d\n", path, IPC_GET_ARG1(answer));
	}

	return NULL;
}
