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
 * Glue code which is common to all FS implementations.
 */

#include "libfs.h"
#include <macros.h>
#include <errno.h>
#include <async.h>
#include <as.h>
#include <assert.h>
#include <dirent.h>
#include <mem.h>
#include <str.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <ipc/vfs.h>
#include <vfs/vfs.h>

#define on_error(rc, action) \
	do { \
		if ((rc) != EOK) \
			action; \
	} while (0)

#define combine_rc(rc1, rc2) \
	((rc1) == EOK ? (rc2) : (rc1))

#define answer_and_return(rid, rc) \
	do { \
		async_answer_0((rid), (rc)); \
		return; \
	} while (0)

static fs_reg_t reg;

static vfs_out_ops_t *vfs_out_ops = NULL;
static libfs_ops_t *libfs_ops = NULL;

static char fs_name[FS_NAME_MAXLEN + 1];

static void libfs_link(libfs_ops_t *, fs_handle_t, ipc_callid_t,
    ipc_call_t *);
static void libfs_lookup(libfs_ops_t *, fs_handle_t, ipc_callid_t,
    ipc_call_t *);
static void libfs_stat(libfs_ops_t *, fs_handle_t, ipc_callid_t, ipc_call_t *);
static void libfs_open_node(libfs_ops_t *, fs_handle_t, ipc_callid_t,
    ipc_call_t *);
static void libfs_statfs(libfs_ops_t *, fs_handle_t, ipc_callid_t,
    ipc_call_t *);

static void vfs_out_fsprobe(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	errno_t rc;
	vfs_fs_probe_info_t info;
	
	ipc_callid_t callid;
	size_t size;
	if ((!async_data_read_receive(&callid, &size)) ||
	    (size != sizeof(info))) {
		async_answer_0(callid, EIO);
		async_answer_0(rid, EIO);
		return;
	}
	
	memset(&info, 0, sizeof(info));
	rc = vfs_out_ops->fsprobe(service_id, &info);
	if (rc != EOK) {
		async_answer_0(callid, EIO);
		async_answer_0(rid, rc);
		return;
	}
	
	async_data_read_finalize(callid, &info, sizeof(info));
	async_answer_0(rid, EOK);
}

static void vfs_out_mounted(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	char *opts;
	errno_t rc;
	
	/* Accept the mount options. */
	rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	fs_index_t index;
	aoff64_t size;
	rc = vfs_out_ops->mounted(service_id, opts, &index, &size);

	if (rc == EOK)
		async_answer_3(rid, EOK, index, LOWER32(size), UPPER32(size));
	else
		async_answer_0(rid, rc);

	free(opts);
}

static void vfs_out_unmounted(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	errno_t rc; 

	rc = vfs_out_ops->unmounted(service_id);

	async_answer_0(rid, rc);
}

static void vfs_out_link(ipc_callid_t rid, ipc_call_t *req)
{
	libfs_link(libfs_ops, reg.fs_handle, rid, req);
}

static void vfs_out_lookup(ipc_callid_t rid, ipc_call_t *req)
{
	libfs_lookup(libfs_ops, reg.fs_handle, rid, req);
}

static void vfs_out_read(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	aoff64_t pos = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*req),
	    IPC_GET_ARG4(*req));
	size_t rbytes;
	errno_t rc;

	rc = vfs_out_ops->read(service_id, index, pos, &rbytes);

	if (rc == EOK)
		async_answer_1(rid, EOK, rbytes);
	else
		async_answer_0(rid, rc);
}

static void vfs_out_write(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	aoff64_t pos = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*req),
	    IPC_GET_ARG4(*req));
	size_t wbytes;
	aoff64_t nsize;
	errno_t rc;

	rc = vfs_out_ops->write(service_id, index, pos, &wbytes, &nsize);

	if (rc == EOK) {
		async_answer_3(rid, EOK, wbytes, LOWER32(nsize),
		    UPPER32(nsize));
	} else
		async_answer_0(rid, rc);
}

static void vfs_out_truncate(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	aoff64_t size = (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*req),
	    IPC_GET_ARG4(*req));
	errno_t rc;

	rc = vfs_out_ops->truncate(service_id, index, size);

	async_answer_0(rid, rc);
}

static void vfs_out_close(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	errno_t rc;

	rc = vfs_out_ops->close(service_id, index);

	async_answer_0(rid, rc);
}

static void vfs_out_destroy(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);

	errno_t rc;
	fs_node_t *node = NULL;
	rc = libfs_ops->node_get(&node, service_id, index);
	if (rc == EOK && node != NULL) {
		bool destroy = (libfs_ops->lnkcnt_get(node) == 0);
		libfs_ops->node_put(node);
		if (destroy)
			rc = vfs_out_ops->destroy(service_id, index);
	}
	async_answer_0(rid, rc);
}

static void vfs_out_open_node(ipc_callid_t rid, ipc_call_t *req)
{
	libfs_open_node(libfs_ops, reg.fs_handle, rid, req);
}

static void vfs_out_stat(ipc_callid_t rid, ipc_call_t *req)
{
	libfs_stat(libfs_ops, reg.fs_handle, rid, req);
}

static void vfs_out_sync(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	errno_t rc;

	rc = vfs_out_ops->sync(service_id, index);

	async_answer_0(rid, rc);
}

static void vfs_out_statfs(ipc_callid_t rid, ipc_call_t *req)
{
	libfs_statfs(libfs_ops, reg.fs_handle, rid, req);
}

static void vfs_out_is_empty(ipc_callid_t rid, ipc_call_t *req)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*req);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*req);
	errno_t rc;

	fs_node_t *node = NULL;
	rc = libfs_ops->node_get(&node, service_id, index);
	if (rc != EOK)
		async_answer_0(rid, rc);
	if (node == NULL)
		async_answer_0(rid, EINVAL);
	
	bool children = false;
	rc = libfs_ops->has_children(&children, node);
	libfs_ops->node_put(node);
	
	if (rc != EOK)
		async_answer_0(rid, rc);
	async_answer_0(rid, children ? ENOTEMPTY : EOK);
}

static void vfs_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	if (iid) {
		/*
		 * This only happens for connections opened by
		 * IPC_M_CONNECT_ME_TO calls as opposed to callback connections
		 * created by IPC_M_CONNECT_TO_ME.
		 */
		async_answer_0(iid, EOK);
	}
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			return;
		
		switch (IPC_GET_IMETHOD(call)) {
		case VFS_OUT_FSPROBE:
			vfs_out_fsprobe(callid, &call);
			break;
		case VFS_OUT_MOUNTED:
			vfs_out_mounted(callid, &call);
			break;
		case VFS_OUT_UNMOUNTED:
			vfs_out_unmounted(callid, &call);
			break;
		case VFS_OUT_LINK:
			vfs_out_link(callid, &call);
			break;
		case VFS_OUT_LOOKUP:
			vfs_out_lookup(callid, &call);
			break;
		case VFS_OUT_READ:
			vfs_out_read(callid, &call);
			break;
		case VFS_OUT_WRITE:
			vfs_out_write(callid, &call);
			break;
		case VFS_OUT_TRUNCATE:
			vfs_out_truncate(callid, &call);
			break;
		case VFS_OUT_CLOSE:
			vfs_out_close(callid, &call);
			break;
		case VFS_OUT_DESTROY:
			vfs_out_destroy(callid, &call);
			break;
		case VFS_OUT_OPEN_NODE:
			vfs_out_open_node(callid, &call);
			break;
		case VFS_OUT_STAT:
			vfs_out_stat(callid, &call);
			break;
		case VFS_OUT_SYNC:
			vfs_out_sync(callid, &call);
			break;
		case VFS_OUT_STATFS:
			vfs_out_statfs(callid, &call);
			break;
		case VFS_OUT_IS_EMPTY:
			vfs_out_is_empty(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

/** Register file system server.
 *
 * This function abstracts away the tedious registration protocol from
 * file system implementations and lets them to reuse this registration glue
 * code.
 *
 * @param sess Session for communication with VFS.
 * @param info VFS info structure supplied by the file system
 *             implementation.
 * @param vops Address of the vfs_out_ops_t structure.
 * @param lops Address of the libfs_ops_t structure.
 *
 * @return EOK on success or a non-zero error code on errror.
 *
 */
errno_t fs_register(async_sess_t *sess, vfs_info_t *info, vfs_out_ops_t *vops,
    libfs_ops_t *lops)
{
	/*
	 * Tell VFS that we are here and want to get registered.
	 * We use the async framework because VFS will answer the request
	 * out-of-order, when it knows that the operation succeeded or failed.
	 */
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, VFS_IN_REGISTER, &answer);
	
	/*
	 * Send our VFS info structure to VFS.
	 */
	errno_t rc = async_data_write_start(exch, info, sizeof(*info));
	
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	/*
	 * Set VFS_OUT and libfs operations.
	 */
	vfs_out_ops = vops;
	libfs_ops = lops;

	str_cpy(fs_name, sizeof(fs_name), info->name);

	/*
	 * Ask VFS for callback connection.
	 */
	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_VFS_DRIVER_CB, 0, 0,
	    vfs_connection, NULL, &port);
	
	/*
	 * Request sharing the Path Lookup Buffer with VFS.
	 */
	rc = async_share_in_start_0_0(exch, PLB_SIZE, (void *) &reg.plb_ro);
	if (reg.plb_ro == AS_MAP_FAILED) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}
	
	async_exchange_end(exch);
	
	if (rc) {
		async_forget(req);
		return rc;
	}
	 
	/*
	 * Pick up the answer for the request to the VFS_IN_REQUEST call.
	 */
	async_wait_for(req, NULL);
	reg.fs_handle = (int) IPC_GET_ARG1(answer);
	
	/*
	 * Tell the async framework that other connections are to be handled by
	 * the same connection fibril as well.
	 */
	async_set_fallback_port_handler(vfs_connection, NULL);
	
	return IPC_GET_RETVAL(answer);
}

void fs_node_initialize(fs_node_t *fn)
{
	memset(fn, 0, sizeof(fs_node_t));
}

static char plb_get_char(unsigned pos)
{
	return reg.plb_ro[pos % PLB_SIZE];
}

static errno_t plb_get_component(char *dest, unsigned *sz, unsigned *ppos,
    unsigned last)
{
	unsigned pos = *ppos;
	unsigned size = 0;
	
	if (pos == last) {
		*sz = 0;
		return ERANGE;
	}

	char c = plb_get_char(pos); 
	if (c == '/')
		pos++;
	
	for (int i = 0; i <= NAME_MAX; i++) {
		c = plb_get_char(pos);
		if (pos == last || c == '/') {
			dest[i] = 0;
			*ppos = pos;
			*sz = size;
			return EOK;
		}
		dest[i] = c;
		pos++;
		size++;
	}
	return ENAMETOOLONG;
}

static errno_t receive_fname(char *buffer)
{
	size_t size;
	ipc_callid_t wcall;
	
	if (!async_data_write_receive(&wcall, &size))
		return ENOENT;
	if (size > NAME_MAX + 1) {
		async_answer_0(wcall, ERANGE);
		return ERANGE;
	}
	return async_data_write_finalize(wcall, buffer, size);
}

/** Link a file at a path.
 */
void libfs_link(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *req)
{
	service_id_t parent_sid = IPC_GET_ARG1(*req);
	fs_index_t parent_index = IPC_GET_ARG2(*req);
	fs_index_t child_index = IPC_GET_ARG3(*req);
	
	char component[NAME_MAX + 1];
	errno_t rc = receive_fname(component);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	fs_node_t *parent = NULL;
	rc = ops->node_get(&parent, parent_sid, parent_index);
	if (parent == NULL) {
		async_answer_0(rid, rc == EOK ? EBADF : rc);
		return;
	}
	
	fs_node_t *child = NULL;
	rc = ops->node_get(&child, parent_sid, child_index);
	if (child == NULL) {
		async_answer_0(rid, rc == EOK ? EBADF : rc);
		ops->node_put(parent);
		return;
	}
	
	rc = ops->link(parent, child, component);
	ops->node_put(parent);
	ops->node_put(child);
	async_answer_0(rid, rc);
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
    ipc_call_t *req)
{
	unsigned first = IPC_GET_ARG1(*req);
	unsigned len = IPC_GET_ARG2(*req);
	service_id_t service_id = IPC_GET_ARG3(*req);
	fs_index_t index = IPC_GET_ARG4(*req);
	int lflag = IPC_GET_ARG5(*req);
	
	// TODO: Validate flags.
	
	unsigned next = first;
	unsigned last = first + len;
	
	char component[NAME_MAX + 1];
	errno_t rc;
	
	fs_node_t *par = NULL;
	fs_node_t *cur = NULL;
	fs_node_t *tmp = NULL;
	unsigned clen = 0;
	
	rc = ops->node_get(&cur, service_id, index);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		goto out;
	}
	
	assert(cur != NULL);
	
	/* Find the file and its parent. */
	
	unsigned last_next = 0;
	
	while (next != last) {
		if (cur == NULL) {
			assert(par != NULL);
			goto out1;
		}

		if (!ops->is_directory(cur)) {
			async_answer_0(rid, ENOTDIR);
			goto out;
		}
		
		last_next = next;
		/* Collect the component */
		rc = plb_get_component(component, &clen, &next, last);
		assert(rc != ERANGE);
		if (rc != EOK) {
			async_answer_0(rid, rc);
			goto out;
		}
		
		if (clen == 0) {
			/* The path is just "/". */
			break;
		}
		
		assert(component[clen] == 0);
		
		/* Match the component */
		rc = ops->match(&tmp, cur, component);
		if (rc != EOK) {
			async_answer_0(rid, rc);
			goto out;
		}
		
		/* Descend one level */
		if (par) {
			rc = ops->node_put(par);
			if (rc != EOK) {
				async_answer_0(rid, rc);
				goto out;
			}
		}
		
		par = cur;
		cur = tmp;
		tmp = NULL;
	}
	
	/* At this point, par is either NULL or a directory.
	 * If cur is NULL, the looked up file does not exist yet.
	 */
	 
	assert(par == NULL || ops->is_directory(par));
	assert(par != NULL || cur != NULL);
	
	/* Check for some error conditions. */
	
	if (cur && (lflag & L_FILE) && (ops->is_directory(cur))) {
		async_answer_0(rid, EISDIR);
		goto out;
	}
	
	if (cur && (lflag & L_DIRECTORY) && (ops->is_file(cur))) {
		async_answer_0(rid, ENOTDIR);
		goto out;
	}
	
	/* Unlink. */
	
	if (lflag & L_UNLINK) {
		if (!cur) {
			async_answer_0(rid, ENOENT);
			goto out;
		}
		if (!par) {
			async_answer_0(rid, EINVAL);
			goto out;
		}
		
		rc = ops->unlink(par, cur, component);
		if (rc == EOK) {
			aoff64_t size = ops->size_get(cur);
			async_answer_5(rid, EOK, fs_handle,
			    ops->index_get(cur),
			    (ops->is_directory(cur) << 16) | last,
			    LOWER32(size), UPPER32(size));
		} else {
			async_answer_0(rid, rc);
		}
		goto out;
	}
	
	/* Create. */
	
	if (lflag & L_CREATE) {
		if (cur && (lflag & L_EXCLUSIVE)) {
			async_answer_0(rid, EEXIST);
			goto out;
		}
	
		if (!cur) {
			rc = ops->create(&cur, service_id,
			    lflag & (L_FILE | L_DIRECTORY));
			if (rc != EOK) {
				async_answer_0(rid, rc);
				goto out;
			}
			if (!cur) {
				async_answer_0(rid, ENOSPC);
				goto out;
			}
			
			rc = ops->link(par, cur, component);
			if (rc != EOK) {
				(void) ops->destroy(cur);
				cur = NULL;
				async_answer_0(rid, rc);
				goto out;
			}
		}
	}
	
	/* Return. */
out1:
	if (!cur) {
		async_answer_5(rid, EOK, fs_handle, ops->index_get(par),
		    (ops->is_directory(par) << 16) | last_next,
		    LOWER32(ops->size_get(par)), UPPER32(ops->size_get(par)));
		goto out;
	}
	
	async_answer_5(rid, EOK, fs_handle, ops->index_get(cur),
	    (ops->is_directory(cur) << 16) | last, LOWER32(ops->size_get(cur)),
	    UPPER32(ops->size_get(cur)));
	
out:
	if (par)
		(void) ops->node_put(par);
	
	if (cur)
		(void) ops->node_put(cur);
	
	if (tmp)
		(void) ops->node_put(tmp);
}

void libfs_stat(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);

	fs_node_t *fn;
	errno_t rc = ops->node_get(&fn, service_id, index);
	on_error(rc, answer_and_return(rid, rc));

	ipc_callid_t callid;
	size_t size;
	if ((!async_data_read_receive(&callid, &size)) ||
	    (size != sizeof(vfs_stat_t))) {
		ops->node_put(fn);
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	vfs_stat_t stat;
	memset(&stat, 0, sizeof(vfs_stat_t));

	stat.fs_handle = fs_handle;
	stat.service_id = service_id;
	stat.index = index;
	stat.lnkcnt = ops->lnkcnt_get(fn);
	stat.is_file = ops->is_file(fn);
	stat.is_directory = ops->is_directory(fn);
	stat.size = ops->size_get(fn);
	stat.service = ops->service_get(fn);

	ops->node_put(fn);


	async_data_read_finalize(callid, &stat, sizeof(vfs_stat_t));
	async_answer_0(rid, EOK);
}

void libfs_statfs(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	service_id_t service_id = (service_id_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);

	fs_node_t *fn;
	errno_t rc = ops->node_get(&fn, service_id, index);
	on_error(rc, answer_and_return(rid, rc));

	ipc_callid_t callid;
	size_t size;
	if ((!async_data_read_receive(&callid, &size)) ||
	    (size != sizeof(vfs_statfs_t))) {
		goto error;
	}

	vfs_statfs_t st;
	memset(&st, 0, sizeof(vfs_statfs_t));

	str_cpy(st.fs_name, sizeof(st.fs_name), fs_name);

	if (ops->size_block != NULL) {
		rc = ops->size_block(service_id, &st.f_bsize);
		if (rc != EOK)
			goto error;
	}

	if (ops->total_block_count != NULL) {
		rc = ops->total_block_count(service_id, &st.f_blocks);
		if (rc != EOK)
			goto error;
	}

	if (ops->free_block_count != NULL) {
		rc = ops->free_block_count(service_id, &st.f_bfree);
		if (rc != EOK)
			goto error;
	}

	ops->node_put(fn);
	async_data_read_finalize(callid, &st, sizeof(vfs_statfs_t));
	async_answer_0(rid, EOK);
	return;

error:
	ops->node_put(fn);
	async_answer_0(callid, EINVAL);
	async_answer_0(rid, EINVAL);
}


/** Open VFS triplet.
 *
 * @param ops     libfs operations structure with function pointers to
 *                file system implementation
 * @param rid     Request ID of the VFS_OUT_OPEN_NODE request.
 * @param request VFS_OUT_OPEN_NODE request data itself.
 *
 */
void libfs_open_node(libfs_ops_t *ops, fs_handle_t fs_handle, ipc_callid_t rid,
    ipc_call_t *request)
{
	service_id_t service_id = IPC_GET_ARG1(*request);
	fs_index_t index = IPC_GET_ARG2(*request);
	
	fs_node_t *fn;
	errno_t rc = ops->node_get(&fn, service_id, index);
	on_error(rc, answer_and_return(rid, rc));
	
	if (fn == NULL) {
		async_answer_0(rid, ENOENT);
		return;
	}
	
	rc = ops->node_open(fn);
	aoff64_t size = ops->size_get(fn);
	async_answer_4(rid, rc, LOWER32(size), UPPER32(size),
	    ops->lnkcnt_get(fn),
	    (ops->is_file(fn) ? L_FILE : 0) |
	    (ops->is_directory(fn) ? L_DIRECTORY : 0));
	
	(void) ops->node_put(fn);
}

static FIBRIL_MUTEX_INITIALIZE(instances_mutex);
static LIST_INITIALIZE(instances_list);

typedef struct {
	service_id_t service_id;
	link_t link;
	void *data;
} fs_instance_t;

errno_t fs_instance_create(service_id_t service_id, void *data)
{
	fs_instance_t *inst = malloc(sizeof(fs_instance_t));
	if (!inst)
		return ENOMEM;

	link_initialize(&inst->link);
	inst->service_id = service_id;
	inst->data = data;

	fibril_mutex_lock(&instances_mutex);
	list_foreach(instances_list, link, fs_instance_t, cur) {
		if (cur->service_id == service_id) {
			fibril_mutex_unlock(&instances_mutex);
			free(inst);
			return EEXIST;
		}

		/* keep the list sorted */
		if (cur->service_id < service_id) {
			list_insert_before(&inst->link, &cur->link);
			fibril_mutex_unlock(&instances_mutex);
			return EOK;
		}
	}
	list_append(&inst->link, &instances_list);
	fibril_mutex_unlock(&instances_mutex);

	return EOK;
}

errno_t fs_instance_get(service_id_t service_id, void **idp)
{
	fibril_mutex_lock(&instances_mutex);
	list_foreach(instances_list, link, fs_instance_t, inst) {
		if (inst->service_id == service_id) {
			*idp = inst->data;
			fibril_mutex_unlock(&instances_mutex);
			return EOK;
		}
	}
	fibril_mutex_unlock(&instances_mutex);
	return ENOENT;
}

errno_t fs_instance_destroy(service_id_t service_id)
{
	fibril_mutex_lock(&instances_mutex);
	list_foreach(instances_list, link, fs_instance_t, inst) {
		if (inst->service_id == service_id) {
			list_remove(&inst->link);
			fibril_mutex_unlock(&instances_mutex);
			free(inst);
			return EOK;
		}
	}
	fibril_mutex_unlock(&instances_mutex);
	return ENOENT;
}

/** @}
 */
