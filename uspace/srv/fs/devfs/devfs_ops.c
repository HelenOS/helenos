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
 * @file devfs_ops.c
 * @brief Implementation of VFS operations for the devfs file system server.
 */

#include <ipc/ipc.h>
#include <bool.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <libfs.h>
#include <fibril_synch.h>
#include <adt/hash_table.h>
#include <ipc/devmap.h>
#include <sys/stat.h>
#include <libfs.h>
#include <assert.h>
#include "devfs.h"
#include "devfs_ops.h"

typedef struct {
	devmap_handle_type_t type;
	dev_handle_t handle;
} devfs_node_t;

/** Opened devices structure */
typedef struct {
	dev_handle_t handle;
	int phone;
	size_t refcount;
	link_t link;
} device_t;

/** Hash table of opened devices */
static hash_table_t devices;

/** Hash table mutex */
static FIBRIL_MUTEX_INITIALIZE(devices_mutex);

#define DEVICES_KEYS        1
#define DEVICES_KEY_HANDLE  0
#define DEVICES_BUCKETS     256

/* Implementation of hash table interface for the nodes hash table. */
static hash_index_t devices_hash(unsigned long key[])
{
	return key[DEVICES_KEY_HANDLE] % DEVICES_BUCKETS;
}

static int devices_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	device_t *dev = hash_table_get_instance(item, device_t, link);
	return (dev->handle == (dev_handle_t) key[DEVICES_KEY_HANDLE]);
}

static void devices_remove_callback(link_t *item)
{
	free(hash_table_get_instance(item, device_t, link));
}

static hash_table_operations_t devices_ops = {
	.hash = devices_hash,
	.compare = devices_compare,
	.remove_callback = devices_remove_callback
};

static int devfs_node_get_internal(fs_node_t **rfn, devmap_handle_type_t type,
    dev_handle_t handle)
{
	devfs_node_t *node = (devfs_node_t *) malloc(sizeof(devfs_node_t));
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
	node->handle = handle;
	
	(*rfn)->data = node;
	return EOK;
}

static int devfs_root_get(fs_node_t **rfn, dev_handle_t dev_handle)
{
	return devfs_node_get_internal(rfn, DEV_HANDLE_NONE, 0);
}

static int devfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	devfs_node_t *node = (devfs_node_t *) pfn->data;
	
	if (node->handle == 0) {
		/* Root directory */
		
		dev_desc_t *devs;
		size_t count = devmap_get_namespaces(&devs);
		
		if (count > 0) {
			size_t pos;
			for (pos = 0; pos < count; pos++) {
				/* Ignore root namespace */
				if (str_cmp(devs[pos].name, "") == 0)
					continue;
				
				if (str_cmp(devs[pos].name, component) == 0) {
					free(devs);
					return devfs_node_get_internal(rfn, DEV_HANDLE_NAMESPACE, devs[pos].handle);
				}
			}
			
			free(devs);
		}
		
		/* Search root namespace */
		dev_handle_t namespace;
		if (devmap_namespace_get_handle("", &namespace, 0) == EOK) {
			count = devmap_get_devices(namespace, &devs);
			
			if (count > 0) {
				size_t pos;
				for (pos = 0; pos < count; pos++) {
					if (str_cmp(devs[pos].name, component) == 0) {
						free(devs);
						return devfs_node_get_internal(rfn, DEV_HANDLE_DEVICE, devs[pos].handle);
					}
				}
				
				free(devs);
			}
		}
		
		*rfn = NULL;
		return EOK;
	}
	
	if (node->type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		
		dev_desc_t *devs;
		size_t count = devmap_get_devices(node->handle, &devs);
		if (count > 0) {
			size_t pos;
			for (pos = 0; pos < count; pos++) {
				if (str_cmp(devs[pos].name, component) == 0) {
					free(devs);
					return devfs_node_get_internal(rfn, DEV_HANDLE_DEVICE, devs[pos].handle);
				}
			}
			
			free(devs);
		}
		
		*rfn = NULL;
		return EOK;
	}
	
	*rfn = NULL;
	return EOK;
}

static int devfs_node_get(fs_node_t **rfn, dev_handle_t dev_handle, fs_index_t index)
{
	return devfs_node_get_internal(rfn, devmap_handle_probe(index), index);
}

static int devfs_node_open(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	if (node->handle == 0) {
		/* Root directory */
		return EOK;
	}
	
	devmap_handle_type_t type = devmap_handle_probe(node->handle);
	
	if (type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		return EOK;
	}
	
	if (type == DEV_HANDLE_DEVICE) {
		/* Device node */
		
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) node->handle
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk == NULL) {
			device_t *dev = (device_t *) malloc(sizeof(device_t));
			if (dev == NULL) {
				fibril_mutex_unlock(&devices_mutex);
				return ENOMEM;
			}
			
			int phone = devmap_device_connect(node->handle, 0);
			if (phone < 0) {
				fibril_mutex_unlock(&devices_mutex);
				free(dev);
				return ENOENT;
			}
			
			dev->handle = node->handle;
			dev->phone = phone;
			dev->refcount = 1;
			
			hash_table_insert(&devices, key, &dev->link);
		} else {
			device_t *dev = hash_table_get_instance(lnk, device_t, link);
			dev->refcount++;
		}
		
		fibril_mutex_unlock(&devices_mutex);
		
		return EOK;
	}
	
	return ENOENT;
}

static int devfs_node_put(fs_node_t *fn)
{
	free(fn->data);
	free(fn);
	return EOK;
}

static int devfs_create_node(fs_node_t **rfn, dev_handle_t dev_handle, int lflag)
{
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));
	
	*rfn = NULL;
	return ENOTSUP;
}

static int devfs_destroy_node(fs_node_t *fn)
{
	return ENOTSUP;
}

static int devfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return ENOTSUP;
}

static int devfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return ENOTSUP;
}

static int devfs_has_children(bool *has_children, fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	if (node->handle == 0) {
		size_t count = devmap_count_namespaces();
		if (count > 0) {
			*has_children = true;
			return EOK;
		}
		
		/* Root namespace */
		dev_handle_t namespace;
		if (devmap_namespace_get_handle("", &namespace, 0) == EOK) {
			count = devmap_count_devices(namespace);
			if (count > 0) {
				*has_children = true;
				return EOK;
			}
		}
		
		*has_children = false;
		return EOK;
	}
	
	if (node->type == DEV_HANDLE_NAMESPACE) {
		size_t count = devmap_count_devices(node->handle);
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

static fs_index_t devfs_index_get(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	return node->handle;
}

static size_t devfs_size_get(fs_node_t *fn)
{
	return 0;
}

static unsigned int devfs_lnkcnt_get(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	if (node->handle == 0)
		return 0;
	
	return 1;
}

static char devfs_plb_get_char(unsigned pos)
{
	return devfs_reg.plb_ro[pos % PLB_SIZE];
}

static bool devfs_is_directory(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	return ((node->type == DEV_HANDLE_NONE) || (node->type == DEV_HANDLE_NAMESPACE));
}

static bool devfs_is_file(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	return (node->type == DEV_HANDLE_DEVICE);
}

static dev_handle_t devfs_device_get(fs_node_t *fn)
{
	devfs_node_t *node = (devfs_node_t *) fn->data;
	
	if (node->type == DEV_HANDLE_DEVICE)
		return node->handle;
	
	return 0;
}

/** libfs operations */
libfs_ops_t devfs_libfs_ops = {
	.root_get = devfs_root_get,
	.match = devfs_match,
	.node_get = devfs_node_get,
	.node_open = devfs_node_open,
	.node_put = devfs_node_put,
	.create = devfs_create_node,
	.destroy = devfs_destroy_node,
	.link = devfs_link_node,
	.unlink = devfs_unlink_node,
	.has_children = devfs_has_children,
	.index_get = devfs_index_get,
	.size_get = devfs_size_get,
	.lnkcnt_get = devfs_lnkcnt_get,
	.plb_get_char = devfs_plb_get_char,
	.is_directory = devfs_is_directory,
	.is_file = devfs_is_file,
	.device_get = devfs_device_get
};

bool devfs_init(void)
{
	if (!hash_table_create(&devices, DEVICES_BUCKETS,
	    DEVICES_KEYS, &devices_ops))
		return false;
	
	if (devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING) < 0)
		return false;
	
	return true;
}

void devfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	char *opts;
	
	/* Accept the mount options */
	ipcarg_t retval = async_data_string_receive(&opts, 0);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		return;
	}
	
	free(opts);
	ipc_answer_3(rid, EOK, 0, 0, 0);
}

void devfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&devfs_libfs_ops, devfs_reg.fs_handle, rid, request);
}

void devfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&devfs_libfs_ops, devfs_reg.fs_handle, rid, request);
}

void devfs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&devfs_libfs_ops, devfs_reg.fs_handle, rid, request);
}

void devfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&devfs_libfs_ops, devfs_reg.fs_handle, rid, request);
}

void devfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	if (index == 0) {
		ipc_callid_t callid;
		size_t size;
		if (!async_data_read_receive(&callid, &size)) {
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			return;
		}
		
		dev_desc_t *desc;
		size_t count = devmap_get_namespaces(&desc);
		
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
			ipc_answer_1(rid, EOK, 1);
			return;
		}
		
		free(desc);
		pos -= count;
		
		/* Search root namespace */
		dev_handle_t namespace;
		if (devmap_namespace_get_handle("", &namespace, 0) == EOK) {
			count = devmap_get_devices(namespace, &desc);
			
			if (pos < count) {
				async_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
				free(desc);
				ipc_answer_1(rid, EOK, 1);
				return;
			}
			
			free(desc);
		}
		
		ipc_answer_0(callid, ENOENT);
		ipc_answer_1(rid, ENOENT, 0);
		return;
	}
	
	devmap_handle_type_t type = devmap_handle_probe(index);
	
	if (type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		ipc_callid_t callid;
		size_t size;
		if (!async_data_read_receive(&callid, &size)) {
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			return;
		}
		
		dev_desc_t *desc;
		size_t count = devmap_get_devices(index, &desc);
		
		if (pos < count) {
			async_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
			free(desc);
			ipc_answer_1(rid, EOK, 1);
			return;
		}
		
		free(desc);
		ipc_answer_0(callid, ENOENT);
		ipc_answer_1(rid, ENOENT, 0);
		return;
	}
	
	if (type == DEV_HANDLE_DEVICE) {
		/* Device node */
		
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) index
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk == NULL) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOENT);
			return;
		}
		
		device_t *dev = hash_table_get_instance(lnk, device_t, link);
		
		ipc_callid_t callid;
		if (!async_data_read_receive(&callid, NULL)) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			return;
		}
		
		/* Make a request at the driver */
		ipc_call_t answer;
		aid_t msg = async_send_3(dev->phone, IPC_GET_METHOD(*request),
		    IPC_GET_ARG1(*request), IPC_GET_ARG2(*request),
		    IPC_GET_ARG3(*request), &answer);
		
		/* Forward the IPC_M_DATA_READ request to the driver */
		ipc_forward_fast(callid, dev->phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
		fibril_mutex_unlock(&devices_mutex);
		
		/* Wait for reply from the driver. */
		ipcarg_t rc;
		async_wait_for(msg, &rc);
		size_t bytes = IPC_GET_ARG1(answer);
		
		/* Driver reply is the final result of the whole operation */
		ipc_answer_1(rid, rc, bytes);
		return;
	}
	
	ipc_answer_0(rid, ENOENT);
}

void devfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	if (index == 0) {
		ipc_answer_0(rid, ENOTSUP);
		return;
	}
	
	devmap_handle_type_t type = devmap_handle_probe(index);
	
	if (type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		ipc_answer_0(rid, ENOTSUP);
		return;
	}
	
	if (type == DEV_HANDLE_DEVICE) {
		/* Device node */
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) index
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk == NULL) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOENT);
			return;
		}
		
		device_t *dev = hash_table_get_instance(lnk, device_t, link);
		
		ipc_callid_t callid;
		if (!async_data_write_receive(&callid, NULL)) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			return;
		}
		
		/* Make a request at the driver */
		ipc_call_t answer;
		aid_t msg = async_send_3(dev->phone, IPC_GET_METHOD(*request),
		    IPC_GET_ARG1(*request), IPC_GET_ARG2(*request),
		    IPC_GET_ARG3(*request), &answer);
		
		/* Forward the IPC_M_DATA_WRITE request to the driver */
		ipc_forward_fast(callid, dev->phone, 0, 0, 0, IPC_FF_ROUTE_FROM_ME);
		
		fibril_mutex_unlock(&devices_mutex);
		
		/* Wait for reply from the driver. */
		ipcarg_t rc;
		async_wait_for(msg, &rc);
		size_t bytes = IPC_GET_ARG1(answer);
		
		/* Driver reply is the final result of the whole operation */
		ipc_answer_1(rid, rc, bytes);
		return;
	}
	
	ipc_answer_0(rid, ENOENT);
}

void devfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void devfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
	if (index == 0) {
		ipc_answer_0(rid, EOK);
		return;
	}
	
	devmap_handle_type_t type = devmap_handle_probe(index);
	
	if (type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		ipc_answer_0(rid, EOK);
		return;
	}
	
	if (type == DEV_HANDLE_DEVICE) {
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) index
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk == NULL) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOENT);
			return;
		}
		
		device_t *dev = hash_table_get_instance(lnk, device_t, link);
		dev->refcount--;
		
		if (dev->refcount == 0) {
			ipc_hangup(dev->phone);
			hash_table_remove(&devices, key, DEVICES_KEYS);
		}
		
		fibril_mutex_unlock(&devices_mutex);
		
		ipc_answer_0(rid, EOK);
		return;
	}
	
	ipc_answer_0(rid, ENOENT);
}

void devfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
	if (index == 0) {
		ipc_answer_0(rid, EOK);
		return;
	}
	
	devmap_handle_type_t type = devmap_handle_probe(index);
	
	if (type == DEV_HANDLE_NAMESPACE) {
		/* Namespace directory */
		ipc_answer_0(rid, EOK);
		return;
	}
	
	if (type == DEV_HANDLE_DEVICE) {
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) index
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk == NULL) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOENT);
			return;
		}
		
		device_t *dev = hash_table_get_instance(lnk, device_t, link);
		
		/* Make a request at the driver */
		ipc_call_t answer;
		aid_t msg = async_send_2(dev->phone, IPC_GET_METHOD(*request),
		    IPC_GET_ARG1(*request), IPC_GET_ARG2(*request), &answer);
		
		fibril_mutex_unlock(&devices_mutex);
		
		/* Wait for reply from the driver */
		ipcarg_t rc;
		async_wait_for(msg, &rc);
		
		/* Driver reply is the final result of the whole operation */
		ipc_answer_0(rid, rc);
		return;
	}
	
	ipc_answer_0(rid, ENOENT);
}

void devfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

/**
 * @}
 */
