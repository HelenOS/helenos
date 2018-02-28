/*
 * Copyright (c) 2009 Martin Decky
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
 * @file locfs_ops.c
 * @brief Implementation of VFS operations for the locfs file system server.
 */

#include <macros.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <libfs.h>
#include <fibril_synch.h>
#include <adt/hash_table.h>
#include <ipc/loc.h>
#include <assert.h>
#include "locfs.h"
#include "locfs_ops.h"

typedef struct {
	loc_object_type_t type;
	service_id_t service_id;
} locfs_node_t;

/** Opened services structure */
typedef struct {
	service_id_t service_id;
	async_sess_t *sess;       /**< If NULL, the structure is incomplete. */
	size_t refcount;
	ht_link_t link;
	fibril_condvar_t cv;      /**< Broadcast when completed. */
} service_t;

/** Hash table of opened services */
static hash_table_t services;

/** Hash table mutex */
static FIBRIL_MUTEX_INITIALIZE(services_mutex);

/* Implementation of hash table interface for the nodes hash table. */

static size_t services_key_hash(void *key)
{
	return *(service_id_t*)key;
}

static size_t services_hash(const ht_link_t *item)
{
	service_t *dev = hash_table_get_inst(item, service_t, link);
	return dev->service_id;
}

static bool services_key_equal(void *key, const ht_link_t *item)
{
	service_t *dev = hash_table_get_inst(item, service_t, link);
	return (dev->service_id == *(service_id_t*)key);
}

static void services_remove_callback(ht_link_t *item)
{
	free(hash_table_get_inst(item, service_t, link));
}

static hash_table_ops_t services_ops = {
	.hash = services_hash,
	.key_hash = services_key_hash,
	.key_equal = services_key_equal,
	.equal = NULL,
	.remove_callback = services_remove_callback
};

static errno_t locfs_node_get_internal(fs_node_t **rfn, loc_object_type_t type,
    service_id_t service_id)
{
	locfs_node_t *node = (locfs_node_t *) malloc(sizeof(locfs_node_t));
	if (node == NULL) {
		*rfn = NULL;
		return ENOMEM;
	}

	*rfn = (fs_node_t *) malloc(sizeof(fs_node_t));
	if (*rfn == NULL) {
		free(node);
		*rfn = NULL;
		return ENOMEM;
	}

	fs_node_initialize(*rfn);
	node->type = type;
	node->service_id = service_id;

	(*rfn)->data = node;
	return EOK;
}

static errno_t locfs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return locfs_node_get_internal(rfn, LOC_OBJECT_NONE, 0);
}

static errno_t locfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	locfs_node_t *node = (locfs_node_t *) pfn->data;
	errno_t ret;

	if (node->service_id == 0) {
		/* Root directory */

		loc_sdesc_t *nspaces;
		size_t count = loc_get_namespaces(&nspaces);

		if (count > 0) {
			size_t pos;
			for (pos = 0; pos < count; pos++) {
				/* Ignore root namespace */
				if (str_cmp(nspaces[pos].name, "") == 0)
					continue;

				if (str_cmp(nspaces[pos].name, component) == 0) {
					ret = locfs_node_get_internal(rfn, LOC_OBJECT_NAMESPACE, nspaces[pos].id);
					free(nspaces);
					return ret;
				}
			}

			free(nspaces);
		}

		/* Search root namespace */
		service_id_t namespace;
		loc_sdesc_t *svcs;
		if (loc_namespace_get_id("", &namespace, 0) == EOK) {
			count = loc_get_services(namespace, &svcs);

			if (count > 0) {
				size_t pos;
				for (pos = 0; pos < count; pos++) {
					if (str_cmp(svcs[pos].name, component) == 0) {
						ret = locfs_node_get_internal(rfn, LOC_OBJECT_SERVICE, svcs[pos].id);
						free(svcs);
						return ret;
					}
				}

				free(svcs);
			}
		}

		*rfn = NULL;
		return EOK;
	}

	if (node->type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */

		loc_sdesc_t *svcs;
		size_t count = loc_get_services(node->service_id, &svcs);
		if (count > 0) {
			size_t pos;
			for (pos = 0; pos < count; pos++) {
				if (str_cmp(svcs[pos].name, component) == 0) {
					ret = locfs_node_get_internal(rfn, LOC_OBJECT_SERVICE, svcs[pos].id);
					free(svcs);
					return ret;
				}
			}

			free(svcs);
		}

		*rfn = NULL;
		return EOK;
	}

	*rfn = NULL;
	return EOK;
}

static errno_t locfs_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	return locfs_node_get_internal(rfn, loc_id_probe(index), index);
}

static errno_t locfs_node_open(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	if (node->service_id == 0) {
		/* Root directory */
		return EOK;
	}

	loc_object_type_t type = loc_id_probe(node->service_id);

	if (type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */
		return EOK;
	}

	if (type == LOC_OBJECT_SERVICE) {
		/* Device node */

		fibril_mutex_lock(&services_mutex);
		ht_link_t *lnk;
restart:
		lnk = hash_table_find(&services, &node->service_id);
		if (lnk == NULL) {
			service_t *dev = (service_t *) malloc(sizeof(service_t));
			if (dev == NULL) {
				fibril_mutex_unlock(&services_mutex);
				return ENOMEM;
			}

			dev->service_id = node->service_id;

			/* Mark as incomplete */
			dev->sess = NULL;
			dev->refcount = 1;
			fibril_condvar_initialize(&dev->cv);

			/*
			 * Insert the incomplete device structure so that other
			 * fibrils will not race with us when we drop the mutex
			 * below.
			 */
			hash_table_insert(&services, &dev->link);

			/*
			 * Drop the mutex to allow recursive locfs requests.
			 */
			fibril_mutex_unlock(&services_mutex);

			async_sess_t *sess = loc_service_connect(node->service_id,
			    INTERFACE_FS, 0);

			fibril_mutex_lock(&services_mutex);

			/*
			 * Notify possible waiters about this device structure
			 * being completed (or destroyed).
			 */
			fibril_condvar_broadcast(&dev->cv);

			if (!sess) {
				/*
				 * Connecting failed, need to remove the
				 * entry and free the device structure.
				 */
				hash_table_remove(&services, &node->service_id);
				fibril_mutex_unlock(&services_mutex);

				return ENOENT;
			}

			/* Set the correct session. */
			dev->sess = sess;
		} else {
			service_t *dev = hash_table_get_inst(lnk, service_t, link);

			if (!dev->sess) {
				/*
				 * Wait until the device structure is completed
				 * and start from the beginning as the device
				 * structure might have entirely disappeared
				 * while we were not holding the mutex in
				 * fibril_condvar_wait().
				 */
				fibril_condvar_wait(&dev->cv, &services_mutex);
				goto restart;
			}

			dev->refcount++;
		}

		fibril_mutex_unlock(&services_mutex);

		return EOK;
	}

	return ENOENT;
}

static errno_t locfs_node_put(fs_node_t *fn)
{
	free(fn->data);
	free(fn);
	return EOK;
}

static errno_t locfs_create_node(fs_node_t **rfn, service_id_t service_id, int lflag)
{
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	*rfn = NULL;
	return ENOTSUP;
}

static errno_t locfs_destroy_node(fs_node_t *fn)
{
	return ENOTSUP;
}

static errno_t locfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return ENOTSUP;
}

static errno_t locfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return ENOTSUP;
}

static errno_t locfs_has_children(bool *has_children, fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	if (node->service_id == 0) {
		size_t count = loc_count_namespaces();
		if (count > 0) {
			*has_children = true;
			return EOK;
		}

		/* Root namespace */
		service_id_t namespace;
		if (loc_namespace_get_id("", &namespace, 0) == EOK) {
			count = loc_count_services(namespace);
			if (count > 0) {
				*has_children = true;
				return EOK;
			}
		}

		*has_children = false;
		return EOK;
	}

	if (node->type == LOC_OBJECT_NAMESPACE) {
		size_t count = loc_count_services(node->service_id);
		if (count > 0) {
			*has_children = true;
			return EOK;
		}

		*has_children = false;
		return EOK;
	}

	*has_children = false;
	return EOK;
}

static fs_index_t locfs_index_get(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;
	return node->service_id;
}

static aoff64_t locfs_size_get(fs_node_t *fn)
{
	return 0;
}

static unsigned int locfs_lnkcnt_get(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	if (node->service_id == 0)
		return 0;

	return 1;
}

static bool locfs_is_directory(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	return ((node->type == LOC_OBJECT_NONE) || (node->type == LOC_OBJECT_NAMESPACE));
}

static bool locfs_is_file(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	return (node->type == LOC_OBJECT_SERVICE);
}

static service_id_t locfs_service_get(fs_node_t *fn)
{
	locfs_node_t *node = (locfs_node_t *) fn->data;

	if (node->type == LOC_OBJECT_SERVICE)
		return node->service_id;

	return 0;
}

/** libfs operations */
libfs_ops_t locfs_libfs_ops = {
	.root_get = locfs_root_get,
	.match = locfs_match,
	.node_get = locfs_node_get,
	.node_open = locfs_node_open,
	.node_put = locfs_node_put,
	.create = locfs_create_node,
	.destroy = locfs_destroy_node,
	.link = locfs_link_node,
	.unlink = locfs_unlink_node,
	.has_children = locfs_has_children,
	.index_get = locfs_index_get,
	.size_get = locfs_size_get,
	.lnkcnt_get = locfs_lnkcnt_get,
	.is_directory = locfs_is_directory,
	.is_file = locfs_is_file,
	.service_get = locfs_service_get
};

bool locfs_init(void)
{
	if (!hash_table_create(&services, 0,  0, &services_ops))
		return false;

	return true;
}

static errno_t locfs_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	return ENOTSUP;
}

static errno_t locfs_mounted(service_id_t service_id, const char *opts,
    fs_index_t *index, aoff64_t *size)
{
	*index = 0;
	*size = 0;
	return EOK;
}

static errno_t locfs_unmounted(service_id_t service_id)
{
	return ENOTSUP;
}

static errno_t
locfs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	if (index == 0) {
		ipc_callid_t callid;
		size_t size;
		if (!async_data_read_receive(&callid, &size)) {
			async_answer_0(callid, EINVAL);
			return EINVAL;
		}

		loc_sdesc_t *desc;
		size_t count = loc_get_namespaces(&desc);

		/* Get rid of root namespace */
		size_t i;
		for (i = 0; i < count; i++) {
			if (str_cmp(desc[i].name, "") == 0) {
				if (pos >= i)
					pos++;

				break;
			}
		}

		if (pos < count) {
			async_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
			free(desc);
			*rbytes = 1;
			return EOK;
		}

		free(desc);
		pos -= count;

		/* Search root namespace */
		service_id_t namespace;
		if (loc_namespace_get_id("", &namespace, 0) == EOK) {
			count = loc_get_services(namespace, &desc);

			if (pos < count) {
				async_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
				free(desc);
				*rbytes = 1;
				return EOK;
			}

			free(desc);
		}

		async_answer_0(callid, ENOENT);
		return ENOENT;
	}

	loc_object_type_t type = loc_id_probe(index);

	if (type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */
		ipc_callid_t callid;
		size_t size;
		if (!async_data_read_receive(&callid, &size)) {
			async_answer_0(callid, EINVAL);
			return EINVAL;
		}

		loc_sdesc_t *desc;
		size_t count = loc_get_services(index, &desc);

		if (pos < count) {
			async_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
			free(desc);
			*rbytes = 1;
			return EOK;
		}

		free(desc);
		async_answer_0(callid, ENOENT);
		return ENOENT;
	}

	if (type == LOC_OBJECT_SERVICE) {
		/* Device node */

		fibril_mutex_lock(&services_mutex);
		service_id_t service_index = index;
		ht_link_t *lnk = hash_table_find(&services, &service_index);
		if (lnk == NULL) {
			fibril_mutex_unlock(&services_mutex);
			return ENOENT;
		}

		service_t *dev = hash_table_get_inst(lnk, service_t, link);
		assert(dev->sess);

		ipc_callid_t callid;
		if (!async_data_read_receive(&callid, NULL)) {
			fibril_mutex_unlock(&services_mutex);
			async_answer_0(callid, EINVAL);
			return EINVAL;
		}

		/* Make a request at the driver */
		async_exch_t *exch = async_exchange_begin(dev->sess);

		ipc_call_t answer;
		aid_t msg = async_send_4(exch, VFS_OUT_READ, service_id,
		    index, LOWER32(pos), UPPER32(pos), &answer);

		/* Forward the IPC_M_DATA_READ request to the driver */
		async_forward_fast(callid, exch, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);

		async_exchange_end(exch);

		fibril_mutex_unlock(&services_mutex);

		/* Wait for reply from the driver. */
		errno_t rc;
		async_wait_for(msg, &rc);

		/* Do not propagate EHANGUP back to VFS. */
		if ((errno_t) rc == EHANGUP)
			rc = ENOTSUP;

		*rbytes = IPC_GET_ARG1(answer);
		return rc;
	}

	return ENOENT;
}

static errno_t
locfs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	if (index == 0)
		return ENOTSUP;

	loc_object_type_t type = loc_id_probe(index);

	if (type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */
		return ENOTSUP;
	}

	if (type == LOC_OBJECT_SERVICE) {
		/* Device node */

		fibril_mutex_lock(&services_mutex);
		service_id_t service_index = index;
		ht_link_t *lnk = hash_table_find(&services, &service_index);
		if (lnk == NULL) {
			fibril_mutex_unlock(&services_mutex);
			return ENOENT;
		}

		service_t *dev = hash_table_get_inst(lnk, service_t, link);
		assert(dev->sess);

		ipc_callid_t callid;
		if (!async_data_write_receive(&callid, NULL)) {
			fibril_mutex_unlock(&services_mutex);
			async_answer_0(callid, EINVAL);
			return EINVAL;
		}

		/* Make a request at the driver */
		async_exch_t *exch = async_exchange_begin(dev->sess);

		ipc_call_t answer;
		aid_t msg = async_send_4(exch, VFS_OUT_WRITE, service_id,
		    index, LOWER32(pos), UPPER32(pos), &answer);

		/* Forward the IPC_M_DATA_WRITE request to the driver */
		async_forward_fast(callid, exch, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);

		async_exchange_end(exch);

		fibril_mutex_unlock(&services_mutex);

		/* Wait for reply from the driver. */
		errno_t rc;
		async_wait_for(msg, &rc);

		/* Do not propagate EHANGUP back to VFS. */
		if ((errno_t) rc == EHANGUP)
			rc = ENOTSUP;

		*wbytes = IPC_GET_ARG1(answer);
		*nsize = 0;
		return rc;
	}

	return ENOENT;
}

static errno_t
locfs_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	return ENOTSUP;
}

static errno_t locfs_close(service_id_t service_id, fs_index_t index)
{
	if (index == 0)
		return EOK;

	loc_object_type_t type = loc_id_probe(index);

	if (type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */
		return EOK;
	}

	if (type == LOC_OBJECT_SERVICE) {

		fibril_mutex_lock(&services_mutex);
		service_id_t service_index = index;
		ht_link_t *lnk = hash_table_find(&services, &service_index);
		if (lnk == NULL) {
			fibril_mutex_unlock(&services_mutex);
			return ENOENT;
		}

		service_t *dev = hash_table_get_inst(lnk, service_t, link);
		assert(dev->sess);
		dev->refcount--;

		if (dev->refcount == 0) {
			async_hangup(dev->sess);
			service_id_t service_index = index;
			hash_table_remove(&services, &service_index);
		}

		fibril_mutex_unlock(&services_mutex);

		return EOK;
	}

	return ENOENT;
}

static errno_t locfs_sync(service_id_t service_id, fs_index_t index)
{
	if (index == 0)
		return EOK;

	loc_object_type_t type = loc_id_probe(index);

	if (type == LOC_OBJECT_NAMESPACE) {
		/* Namespace directory */
		return EOK;
	}

	if (type == LOC_OBJECT_SERVICE) {

		fibril_mutex_lock(&services_mutex);
		service_id_t service_index = index;
		ht_link_t *lnk = hash_table_find(&services, &service_index);
		if (lnk == NULL) {
			fibril_mutex_unlock(&services_mutex);
			return ENOENT;
		}

		service_t *dev = hash_table_get_inst(lnk, service_t, link);
		assert(dev->sess);

		/* Make a request at the driver */
		async_exch_t *exch = async_exchange_begin(dev->sess);

		ipc_call_t answer;
		aid_t msg = async_send_2(exch, VFS_OUT_SYNC, service_id,
		    index, &answer);

		async_exchange_end(exch);

		fibril_mutex_unlock(&services_mutex);

		/* Wait for reply from the driver */
		errno_t rc;
		async_wait_for(msg, &rc);

		return rc;
	}

	return  ENOENT;
}

static errno_t locfs_destroy(service_id_t service_id, fs_index_t index)
{
	return ENOTSUP;
}

vfs_out_ops_t locfs_ops = {
	.fsprobe = locfs_fsprobe,
	.mounted = locfs_mounted,
	.unmounted = locfs_unmounted,
	.read = locfs_read,
	.write = locfs_write,
	.truncate = locfs_truncate,
	.close = locfs_close,
	.destroy = locfs_destroy,
	.sync = locfs_sync,
};

/**
 * @}
 */
