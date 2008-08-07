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

/** Lookup VFS triplet by name in the file system name space.
 *
 * The path passed in the PLB must be in the canonical file system path format
 * as returned by the canonify() function.
 *
 * @param ops		libfs operations structure with function pointers to
 *			file system implementation
 * @param fs_handle	File system handle of the file system where to perform
 *			the lookup.
 * @param rid		Request ID of the VFS_LOOKUP request.
 * @param request	VFS_LOOKUP request data itself.
 */
void libfs_lookup(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	unsigned next = IPC_GET_ARG1(*request);
	unsigned last = IPC_GET_ARG2(*request);
	dev_handle_t dev_handle = IPC_GET_ARG3(*request);
	int lflag = IPC_GET_ARG4(*request);
	fs_index_t index = IPC_GET_ARG5(*request); /* when L_LINK specified */
	char component[NAME_MAX + 1];
	int len;

	if (last < next)
		last += PLB_SIZE;

	void *par = NULL;
	void *cur = ops->root_get(dev_handle);
	void *tmp = NULL;

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
				void *nodep;
				if (lflag & L_CREATE)
					nodep = ops->create(lflag);
				else
					nodep = ops->node_get(dev_handle,
					    index);
				if (nodep) {
					if (!ops->link(cur, nodep, component)) {
						if (lflag & L_CREATE) {
							(void)ops->destroy(
							    nodep);
						}
						ipc_answer_0(rid, ENOSPC);
					} else {
						ipc_answer_5(rid, EOK,
						    fs_handle, dev_handle,
						    ops->index_get(nodep),
						    ops->size_get(nodep),
						    ops->lnkcnt_get(nodep));
						ops->node_put(nodep);
					}
				} else {
					ipc_answer_0(rid, ENOSPC);
				}
				goto out;
			} else if (lflag & L_PARENT) {
				/* return parent */
				ipc_answer_5(rid, EOK, fs_handle, dev_handle,
				    ops->index_get(cur), ops->size_get(cur),
				    ops->lnkcnt_get(cur));
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
				
			void *nodep;
			if (lflag & L_CREATE)
				nodep = ops->create(lflag);
			else
				nodep = ops->node_get(dev_handle, index);
			if (nodep) {
				if (!ops->link(cur, nodep, component)) {
					if (lflag & L_CREATE)
						(void)ops->destroy(nodep);
					ipc_answer_0(rid, ENOSPC);
				} else {
					ipc_answer_5(rid, EOK,
					    fs_handle, dev_handle,
					    ops->index_get(nodep),
					    ops->size_get(nodep),
					    ops->lnkcnt_get(nodep));
					ops->node_put(nodep);
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
	if (lflag & L_PARENT) {
		ops->node_put(cur);
		cur = par;
		par = NULL;
		if (!cur) {
			ipc_answer_0(rid, ENOENT);
			goto out;
		}
	}
	if (lflag & L_UNLINK) {
		unsigned old_lnkcnt = ops->lnkcnt_get(cur);
		int res = ops->unlink(par, cur);
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

#define RD_BASE		1024	// FIXME
#define RD_READ_BLOCK	(RD_BASE + 1)

/** Read data from a block device.
 *
 * @param phone		Phone to be used to communicate with the device.
 * @param buffer	Communication buffer shared with the device.
 * @param bufpos	Pointer to the first unread valid offset within the
 * 			communication buffer.
 * @param buflen	Pointer to the number of unread bytes that are ready in
 * 			the communication buffer.
 * @param pos		Device position to be read.
 * @param dst		Destination buffer.
 * @param size		Size of the destination buffer.
 * @param block_size	Block size to be used for the transfer.
 *
 * @return		True on success, false on failure.
 */
bool libfs_blockread(int phone, void *buffer, off_t *bufpos, size_t *buflen,
    off_t *pos, void *dst, size_t size, size_t block_size)
{
	off_t offset = 0;
	size_t left = size;
	
	while (left > 0) {
		size_t rd;
		
		if (*bufpos + left < *buflen)
			rd = left;
		else
			rd = *buflen - *bufpos;
		
		if (rd > 0) {
			/*
			 * Copy the contents of the communication buffer to the
			 * destination buffer.
			 */
			memcpy(dst + offset, buffer + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			ipcarg_t retval;
			int rc = async_req_2_1(phone, RD_READ_BLOCK,
			    *pos / block_size, block_size, &retval);
			if ((rc != EOK) || (retval != EOK))
				return false;
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	
	return true;
}

/** @}
 */
