/*
 * Copyright (c) 2009 Jakub Jermar 
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
#include <mem.h>
#include <sys/stat.h>

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
	aid_t req = async_send_0(vfs_phone, VFS_IN_REGISTER, &answer);

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
	 * Pick up the answer for the request to the VFS_IN_REQUEST call.
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

void fs_node_initialize(fs_node_t *fn)
{
	memset(fn, 0, sizeof(fs_node_t));
}

void libfs_mount(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	dev_handle_t mp_dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fs_index_t mp_fs_index = (fs_index_t) IPC_GET_ARG2(*request);
	fs_handle_t mr_fs_handle = (fs_handle_t) IPC_GET_ARG3(*request);
	dev_handle_t mr_dev_handle = (dev_handle_t) IPC_GET_ARG4(*request);
	int res;
	ipcarg_t rc;

	ipc_call_t call;
	ipc_callid_t callid;

	/* accept the phone */
	callid = async_get_call(&call);
	int mountee_phone = (int)IPC_GET_ARG1(call);
	if ((IPC_GET_METHOD(call) != IPC_M_CONNECTION_CLONE) ||
	    mountee_phone < 0) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	ipc_answer_0(callid, EOK);	/* acknowledge the mountee_phone */
	
	res = ipc_data_write_receive(&callid, NULL);
	if (!res) {
		ipc_hangup(mountee_phone);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	fs_node_t *fn = ops->node_get(mp_dev_handle, mp_fs_index);
	if (!fn) {
		ipc_hangup(mountee_phone);
		ipc_answer_0(callid, ENOENT);
		ipc_answer_0(rid, ENOENT);
		return;
	}

	if (fn->mp_data.mp_active) {
		ipc_hangup(mountee_phone);
		ops->node_put(fn);
		ipc_answer_0(callid, EBUSY);
		ipc_answer_0(rid, EBUSY);
		return;
	}

	rc = async_req_0_0(mountee_phone, IPC_M_CONNECT_ME);
	if (rc != 0) {
		ipc_hangup(mountee_phone);
		ops->node_put(fn);
		ipc_answer_0(callid, rc);
		ipc_answer_0(rid, rc);
		return;
	}
	
	ipc_call_t answer;
	aid_t msg = async_send_1(mountee_phone, VFS_OUT_MOUNTED, mr_dev_handle,
	    &answer);
	ipc_forward_fast(callid, mountee_phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
	async_wait_for(msg, &rc);
	
	if (rc == EOK) {
		fn->mp_data.mp_active = true;
		fn->mp_data.fs_handle = mr_fs_handle;
		fn->mp_data.dev_handle = mr_dev_handle;
		fn->mp_data.phone = mountee_phone;
	}
	/*
	 * Do not release the FS node so that it stays in memory.
	 */
	ipc_answer_3(rid, rc, IPC_GET_ARG1(answer), IPC_GET_ARG2(answer),
	    IPC_GET_ARG3(answer));
}

/** Lookup VFS triplet by name in the file system name space.
 *
 * The path passed in the PLB must be in the canonical file system path format
 * as returned by the canonify() function.
 *
 * @param ops       libfs operations structure with function pointers to
 *                  file system implementation
 * @param fs_handle File system handle of the file system where to perform
 *                  the lookup.
 * @param rid       Request ID of the VFS_OUT_LOOKUP request.
 * @param request   VFS_OUT_LOOKUP request data itself.
 *
 */
void libfs_lookup(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	unsigned first = IPC_GET_ARG1(*request);
	unsigned last = IPC_GET_ARG2(*request);
	unsigned next = first;
	dev_handle_t dev_handle = IPC_GET_ARG3(*request);
	int lflag = IPC_GET_ARG4(*request);
	fs_index_t index = IPC_GET_ARG5(*request); /* when L_LINK specified */
	char component[NAME_MAX + 1];
	int len;

	if (last < next)
		last += PLB_SIZE;

	fs_node_t *par = NULL;
	fs_node_t *cur = ops->root_get(dev_handle);
	fs_node_t *tmp = NULL;

	if (cur->mp_data.mp_active) {
		ipc_forward_slow(rid, cur->mp_data.phone, VFS_OUT_LOOKUP,
		    next, last, cur->mp_data.dev_handle, lflag, index,
		    IPC_FF_ROUTE_FROM_ME);
		ops->node_put(cur);
		return;
	}

	if (ops->plb_get_char(next) == '/')
		next++;		/* eat slash */
	
	while (next <= last && ops->has_children(cur)) {
		/* collect the component */
		len = 0;
		while ((next <= last) &&  (ops->plb_get_char(next) != '/')) {
			if (len + 1 == NAME_MAX) {
				/* component length overflow */
				ipc_answer_0(rid, ENAMETOOLONG);
				goto out;
			}
			component[len++] = ops->plb_get_char(next);
			next++;	/* process next character */
		}

		assert(len);
		component[len] = '\0';
		next++;		/* eat slash */

		/* match the component */
		tmp = ops->match(cur, component);
		if (tmp && tmp->mp_data.mp_active) {
			if (next > last)
				next = last = first;
			else
				next--;
				
			ipc_forward_slow(rid, tmp->mp_data.phone,
			    VFS_OUT_LOOKUP, next, last, tmp->mp_data.dev_handle,
			    lflag, index, IPC_FF_ROUTE_FROM_ME);
			ops->node_put(cur);
			ops->node_put(tmp);
			if (par)
				ops->node_put(par);
			return;
		}

		/* handle miss: match amongst siblings */
		if (!tmp) {
			if (next <= last) {
				/* there are unprocessed components */
				ipc_answer_0(rid, ENOENT);
				goto out;
			}
			/* miss in the last component */
			if (lflag & (L_CREATE | L_LINK)) { 
				/* request to create a new link */
				if (!ops->is_directory(cur)) {
					ipc_answer_0(rid, ENOTDIR);
					goto out;
				}
				fs_node_t *fn;
				if (lflag & L_CREATE)
					fn = ops->create(dev_handle, lflag);
				else
					fn = ops->node_get(dev_handle,
					    index);
				if (fn) {
					int rc;

					rc = ops->link(cur, fn, component);
					if (rc != EOK) {
						if (lflag & L_CREATE) {
							(void)ops->destroy(fn);
						}
						ipc_answer_0(rid, rc);
					} else {
						ipc_answer_5(rid, EOK,
						    fs_handle, dev_handle,
						    ops->index_get(fn),
						    ops->size_get(fn),
						    ops->lnkcnt_get(fn));
						ops->node_put(fn);
					}
				} else {
					ipc_answer_0(rid, ENOSPC);
				}
				goto out;
			} 
			ipc_answer_0(rid, ENOENT);
			goto out;
		}

		if (par)
			ops->node_put(par);

		/* descend one level */
		par = cur;
		cur = tmp;
		tmp = NULL;
	}

	/* handle miss: excessive components */
	if (next <= last && !ops->has_children(cur)) {
		if (lflag & (L_CREATE | L_LINK)) {
			if (!ops->is_directory(cur)) {
				ipc_answer_0(rid, ENOTDIR);
				goto out;
			}

			/* collect next component */
			len = 0;
			while (next <= last) {
				if (ops->plb_get_char(next) == '/') {
					/* more than one component */
					ipc_answer_0(rid, ENOENT);
					goto out;
				}
				if (len + 1 == NAME_MAX) {
					/* component length overflow */
					ipc_answer_0(rid, ENAMETOOLONG);
					goto out;
				}
				component[len++] = ops->plb_get_char(next);
				next++;	/* process next character */
			}
			assert(len);
			component[len] = '\0';
				
			fs_node_t *fn;
			if (lflag & L_CREATE)
				fn = ops->create(dev_handle, lflag);
			else
				fn = ops->node_get(dev_handle, index);
			if (fn) {
				int rc;

				rc = ops->link(cur, fn, component);
				if (rc != EOK) {
					if (lflag & L_CREATE)
						(void)ops->destroy(fn);
					ipc_answer_0(rid, rc);
				} else {
					ipc_answer_5(rid, EOK,
					    fs_handle, dev_handle,
					    ops->index_get(fn),
					    ops->size_get(fn),
					    ops->lnkcnt_get(fn));
					ops->node_put(fn);
				}
			} else {
				ipc_answer_0(rid, ENOSPC);
			}
			goto out;
		}
		ipc_answer_0(rid, ENOENT);
		goto out;
	}

	/* handle hit */
	if (lflag & L_UNLINK) {
		unsigned old_lnkcnt = ops->lnkcnt_get(cur);
		int res = ops->unlink(par, cur, component);
		ipc_answer_5(rid, (ipcarg_t)res, fs_handle, dev_handle,
		    ops->index_get(cur), ops->size_get(cur), old_lnkcnt);
		goto out;
	}
	if (((lflag & (L_CREATE | L_EXCLUSIVE)) == (L_CREATE | L_EXCLUSIVE)) ||
	    (lflag & L_LINK)) {
		ipc_answer_0(rid, EEXIST);
		goto out;
	}
	if ((lflag & L_FILE) && (ops->is_directory(cur))) {
		ipc_answer_0(rid, EISDIR);
		goto out;
	}
	if ((lflag & L_DIRECTORY) && (ops->is_file(cur))) {
		ipc_answer_0(rid, ENOTDIR);
		goto out;
	}

	ipc_answer_5(rid, EOK, fs_handle, dev_handle, ops->index_get(cur),
	    ops->size_get(cur), ops->lnkcnt_get(cur));

out:
	if (par)
		ops->node_put(par);
	if (cur)
		ops->node_put(cur);
	if (tmp)
		ops->node_put(tmp);
}

void libfs_stat(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	fs_node_t *fn = ops->node_get(dev_handle, index);

	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_read_receive(&callid, &size) ||
	    size < sizeof(struct stat)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	struct stat *stat = malloc(sizeof(struct stat));
	if (!stat) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	memset(stat, 0, sizeof(struct stat));
	
	stat->fs_handle = fs_handle;
	stat->dev_handle = dev_handle;
	stat->index = index;
	stat->lnkcnt = ops->lnkcnt_get(fn); 
	stat->is_file = ops->is_file(fn);
	stat->size = ops->size_get(fn);

	ipc_data_read_finalize(callid, stat, sizeof(struct stat));
	ipc_answer_0(rid, EOK);

	free(stat);
}

/** Open VFS triplet.
 *
 * @param ops       libfs operations structure with function pointers to
 *                  file system implementation
 * @param rid       Request ID of the VFS_OUT_OPEN_NODE request.
 * @param request   VFS_OUT_OPEN_NODE request data itself.
 *
 */
void libfs_open_node(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	dev_handle_t dev_handle = IPC_GET_ARG1(*request);
	fs_index_t index = IPC_GET_ARG2(*request);
	
	fs_node_t *node = ops->node_get(dev_handle, index);
	
	if (node == NULL) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	ipc_answer_5(rid, EOK, fs_handle, dev_handle, index,
	    ops->size_get(node), ops->lnkcnt_get(node));
	
	ops->node_put(node);
}

/** @}
 */
