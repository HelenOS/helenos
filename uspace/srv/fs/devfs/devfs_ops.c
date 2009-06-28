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
#include <fibril_sync.h>
#include <adt/hash_table.h>
#include <sys/stat.h>
#include "devfs.h"
#include "devfs_ops.h"

#define PLB_GET_CHAR(pos)  (devfs_reg.plb_ro[pos % PLB_SIZE])

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
	/* Accept the mount options */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	
	char *opts = malloc(size + 1);
	if (!opts) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	
	ipcarg_t retval = ipc_data_write_finalize(callid, opts, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(opts);
		return;
	}
	
	free(opts);
	
	ipc_answer_3(rid, EOK, 0, 0, 0);
}

void devfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void devfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	ipcarg_t first = IPC_GET_ARG1(*request);
	ipcarg_t last = IPC_GET_ARG2(*request);
	dev_handle_t dev_handle = IPC_GET_ARG3(*request);
	ipcarg_t lflag = IPC_GET_ARG4(*request);
	fs_index_t index = IPC_GET_ARG5(*request);
	
	/* Hierarchy is flat, no altroot is supported */
	if (index != 0) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	if ((lflag & L_LINK) || (lflag & L_UNLINK)) {
		ipc_answer_0(rid, ENOTSUP);
		return;
	}
	
	/* Eat slash */
	if (PLB_GET_CHAR(first) == '/') {
		first++;
		first %= PLB_SIZE;
	}
	
	if (first >= last) {
		/* Root entry */
		if (!(lflag & L_FILE))
			ipc_answer_5(rid, EOK, devfs_reg.fs_handle, dev_handle, 0, 0, 0);
		else
			ipc_answer_0(rid, ENOENT);
	} else {
		if (!(lflag & L_DIRECTORY)) {
			size_t len;
			if (last >= first)
				len = last - first + 1;
			else
				len = first + PLB_SIZE - last + 1;
			
			char *name = (char *) malloc(len + 1);
			if (name == NULL) {
				ipc_answer_0(rid, ENOMEM);
				return;
			}
			
			size_t i;
			for (i = 0; i < len; i++)
				name[i] = PLB_GET_CHAR(first + i);
			
			name[len] = 0;
			
			dev_handle_t handle;
			if (devmap_device_get_handle(name, &handle, 0) != EOK) {
				free(name);
				ipc_answer_0(rid, ENOENT);
				return;
			}
			
			if (lflag & L_OPEN) {
				unsigned long key[] = {
					[DEVICES_KEY_HANDLE] = (unsigned long) handle
				};
				
				fibril_mutex_lock(&devices_mutex);
				link_t *lnk = hash_table_find(&devices, key);
				if (lnk == NULL) {
					int phone = devmap_device_connect(handle, 0);
					if (phone < 0) {
						fibril_mutex_unlock(&devices_mutex);
						free(name);
						ipc_answer_0(rid, ENOENT);
						return;
					}
					
					device_t *dev = (device_t *) malloc(sizeof(device_t));
					if (dev == NULL) {
						fibril_mutex_unlock(&devices_mutex);
						free(name);
						ipc_answer_0(rid, ENOMEM);
						return;
					}
					
					dev->handle = handle;
					dev->phone = phone;
					dev->refcount = 1;
					
					hash_table_insert(&devices, key, &dev->link);
				} else {
					device_t *dev = hash_table_get_instance(lnk, device_t, link);
					dev->refcount++;
				}
				fibril_mutex_unlock(&devices_mutex);
			}
			
			free(name);
			
			ipc_answer_5(rid, EOK, devfs_reg.fs_handle, dev_handle, handle, 0, 1);
		} else
			ipc_answer_0(rid, ENOENT);
	}
}

void devfs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t handle = IPC_GET_ARG2(*request);
	
	unsigned long key[] = {
		[DEVICES_KEY_HANDLE] = (unsigned long) handle
	};
	
	fibril_mutex_lock(&devices_mutex);
	link_t *lnk = hash_table_find(&devices, key);
	if (lnk == NULL) {
		int phone = devmap_device_connect(handle, 0);
		if (phone < 0) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOENT);
			return;
		}
		
		device_t *dev = (device_t *) malloc(sizeof(device_t));
		if (dev == NULL) {
			fibril_mutex_unlock(&devices_mutex);
			ipc_answer_0(rid, ENOMEM);
			return;
		}
		
		dev->handle = handle;
		dev->phone = phone;
		dev->refcount = 1;
		
		hash_table_insert(&devices, key, &dev->link);
	} else {
		device_t *dev = hash_table_get_instance(lnk, device_t, link);
		dev->refcount++;
	}
	fibril_mutex_unlock(&devices_mutex);
	
	ipc_answer_3(rid, EOK, 0, 1, L_FILE);
}

void devfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
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

	stat->fs_handle = devfs_reg.fs_handle;
	stat->dev_handle = dev_handle;
	stat->index = index;
	stat->lnkcnt = 1;
	stat->is_file = (index != 0);
	stat->size = 0;
	
	if (index != 0) {
		unsigned long key[] = {
			[DEVICES_KEY_HANDLE] = (unsigned long) index
		};
		
		fibril_mutex_lock(&devices_mutex);
		link_t *lnk = hash_table_find(&devices, key);
		if (lnk != NULL) 
			stat->devfs_stat.device = (dev_handle_t)index;
		fibril_mutex_unlock(&devices_mutex);
	}

	ipc_data_read_finalize(callid, stat, sizeof(struct stat));
	ipc_answer_0(rid, EOK);

	free(stat);
}

void devfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	if (index != 0) {
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
		if (!ipc_data_read_receive(&callid, NULL)) {
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
	} else {
		ipc_callid_t callid;
		size_t size;
		if (!ipc_data_read_receive(&callid, &size)) {
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			return;
		}
		
		size_t count = devmap_device_get_count();
		dev_desc_t *desc = malloc(count * sizeof(dev_desc_t));
		if (desc == NULL) {
			ipc_answer_0(callid, ENOMEM);
			ipc_answer_1(rid, ENOMEM, 0);
			return;
		}
		
		size_t max = devmap_device_get_devices(count, desc);
		
		if (pos < max) {
			ipc_data_read_finalize(callid, desc[pos].name, str_size(desc[pos].name) + 1);
		} else {
			ipc_answer_0(callid, ENOENT);
			ipc_answer_1(rid, ENOENT, 0);
			return;
		}
		
		free(desc);
		
		ipc_answer_1(rid, EOK, 1);
	}
}

void devfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	off_t pos = (off_t) IPC_GET_ARG3(*request);
	
	if (index != 0) {
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
		if (!ipc_data_write_receive(&callid, NULL)) {
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
	} else {
		/* Read-only filesystem */
		ipc_answer_0(rid, ENOTSUP);
	}
}

void devfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void devfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
	if (index != 0) {
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
	} else
		ipc_answer_0(rid, ENOTSUP);
}

void devfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	
	if (index != 0) {
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
	} else
		ipc_answer_0(rid, ENOTSUP);
}

void devfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

/**
 * @}
 */
