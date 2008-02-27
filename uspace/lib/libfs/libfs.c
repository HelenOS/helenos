/*
 * Copyright (c) 2008 Jakub Jermar 
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
#include <assert.h>
#include <dirent.h>

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
	int rc = ipc_data_write_start(vfs_phone, info, sizeof(*info)); 
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
	rc = ipc_share_in_start_0_0(vfs_phone, reg->plb_ro, PLB_SIZE);
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

void libfs_lookup(libfs_ops_t *ops, int fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	unsigned next = IPC_GET_ARG1(*request);
	unsigned last = IPC_GET_ARG2(*request);
	int dev_handle = IPC_GET_ARG3(*request);
	int lflag = IPC_GET_ARG4(*request);

	if (last < next)
		last += PLB_SIZE;

	void *cur = ops->root_get();
	void *tmp = ops->child_get(cur);

	if (ops->plb_get_char(next) == '/')
		next++;		/* eat slash */
	
	char component[NAME_MAX + 1];
	int len = 0;
	while (tmp && next <= last) {

		/* collect the component */
		if (ops->plb_get_char(next) != '/') {
			if (len + 1 == NAME_MAX) {
				/* comopnent length overflow */
				ipc_answer_0(rid, ENAMETOOLONG);
				return;
			}
			component[len++] = ops->plb_get_char(next);
			next++;	/* process next character */
			if (next <= last) 
				continue;
		}

		assert(len);
		component[len] = '\0';
		next++;		/* eat slash */
		len = 0;

		/* match the component */
		while (tmp && !ops->match(tmp, component))
			tmp = ops->sibling_get(tmp);

		/* handle miss: match amongst siblings */
		if (!tmp) {
			if ((next > last) && (lflag & L_CREATE)) {
				/* no components left and L_CREATE specified */
				if (!ops->is_directory(cur)) {
					ipc_answer_0(rid, ENOTDIR);
					return;
				} 
				void *nodep = ops->create(lflag);
				if (nodep) {
					if (!ops->link(cur, nodep, component)) {
						ops->destroy(nodep);
						ipc_answer_0(rid, ENOSPC);
					} else {
						ipc_answer_5(rid, EOK,
						    fs_handle, dev_handle,
						    ops->index_get(nodep), 0,
						    ops->lnkcnt_get(nodep));
					}
				} else {
					ipc_answer_0(rid, ENOSPC);
				}
				return;
			}
			ipc_answer_0(rid, ENOENT);
			return;
		}

		/* descend one level */
		cur = tmp;
		tmp = ops->child_get(tmp);
	}

	/* handle miss: excessive components */
	if (!tmp && next <= last) {
		if (lflag & L_CREATE) {
			if (!ops->is_directory(cur)) {
				ipc_answer_0(rid, ENOTDIR);
				return;
			}

			/* collect next component */
			while (next <= last) {
				if (ops->plb_get_char(next) == '/') {
					/* more than one component */
					ipc_answer_0(rid, ENOENT);
					return;
				}
				if (len + 1 == NAME_MAX) {
					/* component length overflow */
					ipc_answer_0(rid, ENAMETOOLONG);
					return;
				}
				component[len++] = ops->plb_get_char(next);
				next++;	/* process next character */
			}
			assert(len);
			component[len] = '\0';
			len = 0;
				
			void *nodep = ops->create(lflag);
			if (nodep) {
				if (!ops->link(cur, nodep, component)) {
					ops->destroy(nodep);
					ipc_answer_0(rid, ENOSPC);
				} else {
					ipc_answer_5(rid, EOK,
					    fs_handle, dev_handle,
					    ops->index_get(nodep), 0,
					    ops->lnkcnt_get(nodep));
				}
			} else {
				ipc_answer_0(rid, ENOSPC);
			}
			return;
		}
		ipc_answer_0(rid, ENOENT);
		return;
	}

	/* handle hit */
	if (lflag & L_DESTROY) {
		unsigned old_lnkcnt = ops->lnkcnt_get(cur);
		int res = ops->unlink(cur);
		ipc_answer_5(rid, (ipcarg_t)res, fs_handle, dev_handle,
		    ops->index_get(cur), ops->size_get(cur), old_lnkcnt);
		return;
	}
	if ((lflag & (L_CREATE | L_EXCLUSIVE)) == (L_CREATE | L_EXCLUSIVE)) {
		ipc_answer_0(rid, EEXIST);
		return;
	}
	if ((lflag & L_FILE) && (ops->is_directory(cur))) {
		ipc_answer_0(rid, EISDIR);
		return;
	}
	if ((lflag & L_DIRECTORY) && (ops->is_file(cur))) {
		ipc_answer_0(rid, ENOTDIR);
		return;
	}

	ipc_answer_5(rid, EOK, fs_handle, dev_handle, ops->index_get(cur),
	    ops->size_get(cur), ops->lnkcnt_get(cur));
}

/** @}
 */
