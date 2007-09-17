/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	fat.c
 * @brief	FAT file system driver for HelenOS.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <errno.h>
#include <unistd.h>
#include "../../vfs/vfs.h"

vfs_info_t fat_vfs_info = {
	.name = "fat",
	.ops = {
		[IPC_METHOD_TO_VFS_OP(VFS_REGISTER)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_MOUNT)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_UNMOUNT)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_LOOKUP)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_OPEN)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_CREATE)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_CLOSE)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_READ)] = VFS_OP_DEFINED,
		[IPC_METHOD_TO_VFS_OP(VFS_WRITE)] = VFS_OP_NULL,
		[IPC_METHOD_TO_VFS_OP(VFS_SEEK)] = VFS_OP_DEFAULT
	}
};

int main(int argc, char **argv)
{
	ipcarg_t vfs_phone;

	vfs_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VFS, 0);
	while (vfs_phone != EOK) {
		usleep(10000);
		vfs_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VFS, 0);
	}
	
	/* TODO: start making calls according to the VFS protocol */

	return 0;
}

/**
 * @}
 */ 
