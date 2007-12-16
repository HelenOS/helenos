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

/** @addtogroup libfs 
 * @{
 */ 
/**
 * @file
 * Glue code which is commonod to all FS implementations. 
 */

#include "libfs.h" 
#include "../../srv/vfs/vfs.h"
#include <errno.h>
#include <async.h>
#include <ipc/ipc.h>
#include <as.h>

/** Register file system server.
 *
 * This function abstracts away the tedious registration protocol from
 * file system implementations and lets them to reuse this registration glue
 * code.
 *
 * @param vfs_phone	Open phone for communication with VFS.
 * @param reg		File system registration structure. It will be
 * 			initialized by this function.
 * @param info		VFS info structure supplied by the file system
 *			implementation.
 * @param conn		Connection fibril for handling all calls originating in
 *			VFS.
 *
 * @return		EOK on success or a non-zero error code on errror.
 */
int fs_register(int vfs_phone, fs_reg_t *reg, vfs_info_t *info,
    async_client_conn_t conn)
{
	/*
	 * Tell VFS that we are here and want to get registered.
	 * We use the async framework because VFS will answer the request
	 * out-of-order, when it knows that the operation succeeded or failed.
	 */
	ipc_call_t answer;
	aid_t req = async_send_0(vfs_phone, VFS_REGISTER, &answer);

	/*
	 * Send our VFS info structure to VFS.
	 */
	int rc = ipc_data_send(vfs_phone, info, sizeof(*info)); 
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}

	/*
	 * Ask VFS for callback connection.
	 */
	ipc_connect_to_me(vfs_phone, 0, 0, 0, &reg->vfs_phonehash);

	/*
	 * Allocate piece of address space for PLB.
	 */
	reg->plb_ro = as_get_mappable_page(PLB_SIZE);
	if (!reg->plb_ro) {
		async_wait_for(req, NULL);
		return ENOMEM;
	}

	/*
	 * Request sharing the Path Lookup Buffer with VFS.
	 */
	rc = ipc_call_sync_2_0(vfs_phone, IPC_M_AS_AREA_RECV,
	    (ipcarg_t) reg->plb_ro, PLB_SIZE);
	if (rc) {
		async_wait_for(req, NULL);
		return rc;
	}
	 
	/*
	 * Pick up the answer for the request to the VFS_REQUEST call.
	 */
	async_wait_for(req, NULL);
	reg->fs_handle = (int) IPC_GET_ARG1(answer);
	
	/*
	 * Create a connection fibril to handle the callback connection.
	 */
	async_new_connection(reg->vfs_phonehash, 0, NULL, conn);
	
	/*
	 * Tell the async framework that other connections are to be handled by
	 * the same connection fibril as well.
	 */
	async_set_client_connection(conn);

	return IPC_GET_RETVAL(answer);
}

/** @}
 */
