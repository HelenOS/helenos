/*
 * Copyright (c) 2007 Josef Cejka
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

/**
 * @defgroup devmap Device mapper.
 * @brief HelenOS device mapper.
 * @{
 */

/** @file
 */

#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ipc/devmap.h>
#include <assert.h>

#define NAME          "devmap"
#define NULL_DEVICES  256

/** Representation of device driver.
 *
 * Each driver is responsible for a set of devices.
 *
 */
typedef struct {
	/** Pointers to previous and next drivers in linked list */
	link_t drivers;
	
	/** Pointer to the linked list of devices controlled by this driver */
	link_t devices;
	
	/** Session asociated with this driver */
	async_sess_t *sess;
	
	/** Device driver name */
	char *name;
	
	/** Fibril mutex for list of devices owned by this driver */
	fibril_mutex_t devices_mutex;
} devmap_driver_t;

/** Info about registered namespaces
 *
 */
typedef struct {
	/** Pointer to the previous and next device in the list of all namespaces */
	link_t namespaces;
	
	/** Unique namespace identifier */
	devmap_handle_t handle;
	
	/** Namespace name */
	char *name;
	
	/** Reference count */
	size_t refcnt;
} devmap_namespace_t;

/** Info about registered device
 *
 */
typedef struct {
	/** Pointer to the previous and next device in the list of all devices */
	link_t devices;
	/** Pointer to the previous and next device in the list of devices
	    owned by one driver */
	link_t driver_devices;
	/** Unique device identifier */
	devmap_handle_t handle;
	/** Device namespace */
	devmap_namespace_t *namespace;
	/** Device name */
	char *name;
	/** Device driver handling this device */
	devmap_driver_t *driver;
	/** Use this interface when forwarding to driver. */
	sysarg_t forward_interface;
} devmap_device_t;

LIST_INITIALIZE(devices_list);
LIST_INITIALIZE(namespaces_list);
LIST_INITIALIZE(drivers_list);

/* Locking order:
 *  drivers_list_mutex
 *  devices_list_mutex
 *  (devmap_driver_t *)->devices_mutex
 *  create_handle_mutex
 **/

static FIBRIL_MUTEX_INITIALIZE(devices_list_mutex);
static FIBRIL_CONDVAR_INITIALIZE(devices_list_cv);
static FIBRIL_MUTEX_INITIALIZE(drivers_list_mutex);
static FIBRIL_MUTEX_INITIALIZE(create_handle_mutex);
static FIBRIL_MUTEX_INITIALIZE(null_devices_mutex);

static devmap_handle_t last_handle = 0;
static devmap_device_t *null_devices[NULL_DEVICES];

/*
 * Dummy list for null devices. This is necessary so that null devices can
 * be used just as any other devices, e.g. in devmap_device_unregister_core().
 */
static LIST_INITIALIZE(dummy_null_driver_devices);

static devmap_handle_t devmap_create_handle(void)
{
	/* TODO: allow reusing old handles after their unregistration
	 * and implement some version of LRU algorithm, avoid overflow
	 */
	
	fibril_mutex_lock(&create_handle_mutex);
	last_handle++;
	fibril_mutex_unlock(&create_handle_mutex);
	
	return last_handle;
}

/** Convert fully qualified device name to namespace and device name.
 *
 * A fully qualified device name can be either a plain device name
 * (then the namespace is considered to be an empty string) or consist
 * of two components separated by a slash. No more than one slash
 * is allowed.
 *
 */
static bool devmap_fqdn_split(const char *fqdn, char **ns_name, char **name)
{
	size_t cnt = 0;
	size_t slash_offset = 0;
	size_t slash_after = 0;
	
	size_t offset = 0;
	size_t offset_prev = 0;
	wchar_t c;
	
	while ((c = str_decode(fqdn, &offset, STR_NO_LIMIT)) != 0) {
		if (c == '/') {
			cnt++;
			slash_offset = offset_prev;
			slash_after = offset;
		}
		offset_prev = offset;
	}
	
	/* More than one slash */
	if (cnt > 1)
		return false;
	
	/* No slash -> namespace is empty */
	if (cnt == 0) {
		*ns_name = str_dup("");
		if (*ns_name == NULL)
			return false;
		
		*name = str_dup(fqdn);
		if (*name == NULL) {
			free(*ns_name);
			return false;
		}
		
		if (str_cmp(*name, "") == 0) {
			free(*name);
			free(*ns_name);
			return false;
		}
		
		return true;
	}
	
	/* Exactly one slash */
	*ns_name = str_ndup(fqdn, slash_offset);
	if (*ns_name == NULL)
		return false;
	
	*name = str_dup(fqdn + slash_after);
	if (*name == NULL) {
		free(*ns_name);
		return false;
	}
	
	if (str_cmp(*name, "") == 0) {
		free(*name);
		free(*ns_name);
		return false;
	}
	
	return true;
}

/** Find namespace with given name. */
static devmap_namespace_t *devmap_namespace_find_name(const char *name)
{
	link_t *item;
	
	assert(fibril_mutex_is_locked(&devices_list_mutex));
	
	for (item = namespaces_list.next; item != &namespaces_list; item = item->next) {
		devmap_namespace_t *namespace =
		    list_get_instance(item, devmap_namespace_t, namespaces);
		if (str_cmp(namespace->name, name) == 0)
			return namespace;
	}
	
	return NULL;
}

/** Find namespace with given handle.
 *
 * @todo: use hash table
 *
 */
static devmap_namespace_t *devmap_namespace_find_handle(devmap_handle_t handle)
{
	link_t *item;
	
	assert(fibril_mutex_is_locked(&devices_list_mutex));
	
	for (item = namespaces_list.next; item != &namespaces_list; item = item->next) {
		devmap_namespace_t *namespace =
		    list_get_instance(item, devmap_namespace_t, namespaces);
		if (namespace->handle == handle)
			return namespace;
	}
	
	return NULL;
}

/** Find device with given name. */
static devmap_device_t *devmap_device_find_name(const char *ns_name,
    const char *name)
{
	link_t *item;
	
	assert(fibril_mutex_is_locked(&devices_list_mutex));
	
	for (item = devices_list.next; item != &devices_list; item = item->next) {
		devmap_device_t *device =
		    list_get_instance(item, devmap_device_t, devices);
		if ((str_cmp(device->namespace->name, ns_name) == 0)
		    && (str_cmp(device->name, name) == 0))
			return device;
	}
	
	return NULL;
}

/** Find device with given handle.
 *
 * @todo: use hash table
 *
 */
static devmap_device_t *devmap_device_find_handle(devmap_handle_t handle)
{
	link_t *item;
	
	assert(fibril_mutex_is_locked(&devices_list_mutex));
	
	for (item = devices_list.next; item != &devices_list; item = item->next) {
		devmap_device_t *device =
		    list_get_instance(item, devmap_device_t, devices);
		if (device->handle == handle)
			return device;
	}
	
	return NULL;
}

/** Create a namespace (if not already present). */
static devmap_namespace_t *devmap_namespace_create(const char *ns_name)
{
	devmap_namespace_t *namespace;
	
	assert(fibril_mutex_is_locked(&devices_list_mutex));
	
	namespace = devmap_namespace_find_name(ns_name);
	if (namespace != NULL)
		return namespace;
	
	namespace = (devmap_namespace_t *) malloc(sizeof(devmap_namespace_t));
	if (namespace == NULL)
		return NULL;
	
	namespace->name = str_dup(ns_name);
	if (namespace->name == NULL) {
		free(namespace);
		return NULL;
	}
	
	namespace->handle = devmap_create_handle();
	namespace->refcnt = 0;
	
	/*
	 * Insert new namespace into list of registered namespaces
	 */
	list_append(&(namespace->namespaces), &namespaces_list);
	
	return namespace;
}

/** Destroy a namespace (if it is no longer needed). */
static void devmap_namespace_destroy(devmap_namespace_t *namespace)
{
	assert(fibril_mutex_is_locked(&devices_list_mutex));

	if (namespace->refcnt == 0) {
		list_remove(&(namespace->namespaces));
		
		free(namespace->name);
		free(namespace);
	}
}

/** Increase namespace reference count by including device. */
static void devmap_namespace_addref(devmap_namespace_t *namespace,
    devmap_device_t *device)
{
	assert(fibril_mutex_is_locked(&devices_list_mutex));

	device->namespace = namespace;
	namespace->refcnt++;
}

/** Decrease namespace reference count. */
static void devmap_namespace_delref(devmap_namespace_t *namespace)
{
	assert(fibril_mutex_is_locked(&devices_list_mutex));

	namespace->refcnt--;
	devmap_namespace_destroy(namespace);
}

/** Unregister device and free it. */
static void devmap_device_unregister_core(devmap_device_t *device)
{
	assert(fibril_mutex_is_locked(&devices_list_mutex));

	devmap_namespace_delref(device->namespace);
	list_remove(&(device->devices));
	list_remove(&(device->driver_devices));
	
	free(device->name);
	free(device);
}

/**
 * Read info about new driver and add it into linked list of registered
 * drivers.
 */
static devmap_driver_t *devmap_driver_register(void)
{
	ipc_call_t icall;
	ipc_callid_t iid = async_get_call(&icall);
	
	if (IPC_GET_IMETHOD(icall) != DEVMAP_DRIVER_REGISTER) {
		async_answer_0(iid, EREFUSED);
		return NULL;
	}
	
	devmap_driver_t *driver =
	    (devmap_driver_t *) malloc(sizeof(devmap_driver_t));
	if (driver == NULL) {
		async_answer_0(iid, ENOMEM);
		return NULL;
	}
	
	/*
	 * Get driver name
	 */
	int rc = async_data_write_accept((void **) &driver->name, true, 0,
	    DEVMAP_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(driver);
		async_answer_0(iid, rc);
		return NULL;
	}
	
	/*
	 * Create connection to the driver
	 */
	driver->sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (!driver->sess) {
		free(driver->name);
		free(driver);
		async_answer_0(iid, ENOTSUP);
		return NULL;
	}
	
	/*
	 * Initialize mutex for list of devices
	 * owned by this driver
	 */
	fibril_mutex_initialize(&driver->devices_mutex);
	
	/*
	 * Initialize list of asociated devices
	 */
	list_initialize(&driver->devices);

	link_initialize(&driver->drivers);
	
	fibril_mutex_lock(&drivers_list_mutex);
	
	/* TODO:
	 * Check that no driver with name equal to
	 * driver->name is registered
	 */
	
	/*
	 * Insert new driver into list of registered drivers
	 */
	list_append(&(driver->drivers), &drivers_list);
	fibril_mutex_unlock(&drivers_list_mutex);
	
	async_answer_0(iid, EOK);
	
	return driver;
}

/**
 * Unregister device driver, unregister all its devices and free driver
 * structure.
 *
 */
static int devmap_driver_unregister(devmap_driver_t *driver)
{
	if (driver == NULL)
		return EEXISTS;
	
	fibril_mutex_lock(&drivers_list_mutex);
	
	if (driver->sess)
		async_hangup(driver->sess);
	
	/* Remove it from list of drivers */
	list_remove(&(driver->drivers));
	
	/* Unregister all its devices */
	fibril_mutex_lock(&devices_list_mutex);
	fibril_mutex_lock(&driver->devices_mutex);
	
	while (!list_empty(&(driver->devices))) {
		devmap_device_t *device = list_get_instance(driver->devices.next,
		    devmap_device_t, driver_devices);
		devmap_device_unregister_core(device);
	}
	
	fibril_mutex_unlock(&driver->devices_mutex);
	fibril_mutex_unlock(&devices_list_mutex);
	fibril_mutex_unlock(&drivers_list_mutex);
	
	/* Free name and driver */
	if (driver->name != NULL)
		free(driver->name);
	
	free(driver);
	
	return EOK;
}

/** Register instance of device
 *
 */
static void devmap_device_register(ipc_callid_t iid, ipc_call_t *icall,
    devmap_driver_t *driver)
{
	if (driver == NULL) {
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	/* Create new device entry */
	devmap_device_t *device =
	    (devmap_device_t *) malloc(sizeof(devmap_device_t));
	if (device == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	/* Set the interface, if any. */
	device->forward_interface = IPC_GET_ARG1(*icall);

	/* Get fqdn */
	char *fqdn;
	int rc = async_data_write_accept((void **) &fqdn, true, 0,
	    DEVMAP_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(device);
		async_answer_0(iid, rc);
		return;
	}
	
	char *ns_name;
	if (!devmap_fqdn_split(fqdn, &ns_name, &device->name)) {
		free(fqdn);
		free(device);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	free(fqdn);
	
	fibril_mutex_lock(&devices_list_mutex);
	
	devmap_namespace_t *namespace = devmap_namespace_create(ns_name);
	free(ns_name);
	if (namespace == NULL) {
		fibril_mutex_unlock(&devices_list_mutex);
		free(device->name);
		free(device);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&device->devices);
	link_initialize(&device->driver_devices);
	
	/* Check that device is not already registered */
	if (devmap_device_find_name(namespace->name, device->name) != NULL) {
		printf("%s: Device '%s/%s' already registered\n", NAME,
		    namespace->name, device->name);
		devmap_namespace_destroy(namespace);
		fibril_mutex_unlock(&devices_list_mutex);
		free(device->name);
		free(device);
		async_answer_0(iid, EEXISTS);
		return;
	}
	
	/* Get unique device handle */
	device->handle = devmap_create_handle();

	devmap_namespace_addref(namespace, device);
	device->driver = driver;
	
	/* Insert device into list of all devices  */
	list_append(&device->devices, &devices_list);
	
	/* Insert device into list of devices that belog to one driver */
	fibril_mutex_lock(&device->driver->devices_mutex);
	
	list_append(&device->driver_devices, &device->driver->devices);
	
	fibril_mutex_unlock(&device->driver->devices_mutex);
	fibril_condvar_broadcast(&devices_list_cv);
	fibril_mutex_unlock(&devices_list_mutex);
	
	async_answer_1(iid, EOK, device->handle);
}

/**
 *
 */
static int devmap_device_unregister(ipc_callid_t iid, ipc_call_t *icall, 
    devmap_driver_t *driver)
{
	/* TODO */
	return EOK;
}

/** Connect client to the device.
 *
 * Find device driver owning requested device and forward
 * the message to it.
 *
 */
static void devmap_forward(ipc_callid_t callid, ipc_call_t *call)
{
	fibril_mutex_lock(&devices_list_mutex);
	
	/*
	 * Get handle from request
	 */
	devmap_handle_t handle = IPC_GET_ARG2(*call);
	devmap_device_t *dev = devmap_device_find_handle(handle);
	
	if ((dev == NULL) || (dev->driver == NULL) || (!dev->driver->sess)) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	async_exch_t *exch = async_exchange_begin(dev->driver->sess);
	
	if (dev->forward_interface == 0)
		async_forward_fast(callid, exch, dev->handle, 0, 0, IPC_FF_NONE);
	else
		async_forward_fast(callid, exch, dev->forward_interface,
		    dev->handle, 0, IPC_FF_NONE);
	
	async_exchange_end(exch);
	
	fibril_mutex_unlock(&devices_list_mutex);
}

/** Find handle for device instance identified by name.
 *
 * In answer will be send EOK and device handle in arg1 or a error
 * code from errno.h.
 *
 */
static void devmap_device_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *fqdn;
	
	/* Get fqdn */
	int rc = async_data_write_accept((void **) &fqdn, true, 0,
	    DEVMAP_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	char *ns_name;
	char *name;
	if (!devmap_fqdn_split(fqdn, &ns_name, &name)) {
		free(fqdn);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	free(fqdn);
	
	fibril_mutex_lock(&devices_list_mutex);
	const devmap_device_t *dev;
	
recheck:
	
	/*
	 * Find device name in the list of known devices.
	 */
	dev = devmap_device_find_name(ns_name, name);
	
	/*
	 * Device was not found.
	 */
	if (dev == NULL) {
		if (IPC_GET_ARG1(*icall) & IPC_FLAG_BLOCKING) {
			/* Blocking lookup */
			fibril_condvar_wait(&devices_list_cv,
			    &devices_list_mutex);
			goto recheck;
		}
		
		async_answer_0(iid, ENOENT);
		free(ns_name);
		free(name);
		fibril_mutex_unlock(&devices_list_mutex);
		return;
	}
	
	async_answer_1(iid, EOK, dev->handle);
	
	fibril_mutex_unlock(&devices_list_mutex);
	free(ns_name);
	free(name);
}

/** Find handle for namespace identified by name.
 *
 * In answer will be send EOK and device handle in arg1 or a error
 * code from errno.h.
 *
 */
static void devmap_namespace_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *name;
	
	/* Get device name */
	int rc = async_data_write_accept((void **) &name, true, 0,
	    DEVMAP_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	fibril_mutex_lock(&devices_list_mutex);
	const devmap_namespace_t *namespace;
	
recheck:
	
	/*
	 * Find namespace name in the list of known namespaces.
	 */
	namespace = devmap_namespace_find_name(name);
	
	/*
	 * Namespace was not found.
	 */
	if (namespace == NULL) {
		if (IPC_GET_ARG1(*icall) & IPC_FLAG_BLOCKING) {
			/* Blocking lookup */
			fibril_condvar_wait(&devices_list_cv,
			    &devices_list_mutex);
			goto recheck;
		}
		
		async_answer_0(iid, ENOENT);
		free(name);
		fibril_mutex_unlock(&devices_list_mutex);
		return;
	}
	
	async_answer_1(iid, EOK, namespace->handle);
	
	fibril_mutex_unlock(&devices_list_mutex);
	free(name);
}

static void devmap_handle_probe(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&devices_list_mutex);
	
	devmap_namespace_t *namespace =
	    devmap_namespace_find_handle(IPC_GET_ARG1(*icall));
	if (namespace == NULL) {
		devmap_device_t *dev =
		    devmap_device_find_handle(IPC_GET_ARG1(*icall));
		if (dev == NULL)
			async_answer_1(iid, EOK, DEV_HANDLE_NONE);
		else
			async_answer_1(iid, EOK, DEV_HANDLE_DEVICE);
	} else
		async_answer_1(iid, EOK, DEV_HANDLE_NAMESPACE);
	
	fibril_mutex_unlock(&devices_list_mutex);
}

static void devmap_get_namespace_count(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&devices_list_mutex);
	async_answer_1(iid, EOK, list_count(&namespaces_list));
	fibril_mutex_unlock(&devices_list_mutex);
}

static void devmap_get_device_count(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&devices_list_mutex);
	
	devmap_namespace_t *namespace =
	    devmap_namespace_find_handle(IPC_GET_ARG1(*icall));
	if (namespace == NULL)
		async_answer_0(iid, EEXISTS);
	else
		async_answer_1(iid, EOK, namespace->refcnt);
	
	fibril_mutex_unlock(&devices_list_mutex);
}

static void devmap_get_namespaces(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if ((size % sizeof(dev_desc_t)) != 0) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	fibril_mutex_lock(&devices_list_mutex);
	
	size_t count = size / sizeof(dev_desc_t);
	if (count != list_count(&namespaces_list)) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, EOVERFLOW);
		async_answer_0(iid, EOVERFLOW);
		return;
	}
	
	dev_desc_t *desc = (dev_desc_t *) malloc(size);
	if (desc == NULL) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_t *item;
	size_t pos = 0;
	for (item = namespaces_list.next; item != &namespaces_list;
	    item = item->next) {
		devmap_namespace_t *namespace =
		    list_get_instance(item, devmap_namespace_t, namespaces);
		
		desc[pos].handle = namespace->handle;
		str_cpy(desc[pos].name, DEVMAP_NAME_MAXLEN, namespace->name);
		pos++;
	}
	
	sysarg_t retval = async_data_read_finalize(callid, desc, size);
	
	free(desc);
	fibril_mutex_unlock(&devices_list_mutex);
	
	async_answer_0(iid, retval);
}

static void devmap_get_devices(ipc_callid_t iid, ipc_call_t *icall)
{
	/* FIXME: Use faster algorithm which can make better use
	   of namespaces */
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if ((size % sizeof(dev_desc_t)) != 0) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	fibril_mutex_lock(&devices_list_mutex);
	
	devmap_namespace_t *namespace =
	    devmap_namespace_find_handle(IPC_GET_ARG1(*icall));
	if (namespace == NULL) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, ENOENT);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	size_t count = size / sizeof(dev_desc_t);
	if (count != namespace->refcnt) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, EOVERFLOW);
		async_answer_0(iid, EOVERFLOW);
		return;
	}
	
	dev_desc_t *desc = (dev_desc_t *) malloc(size);
	if (desc == NULL) {
		fibril_mutex_unlock(&devices_list_mutex);
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	link_t *item;
	size_t pos = 0;
	for (item = devices_list.next; item != &devices_list; item = item->next) {
		devmap_device_t *device =
		    list_get_instance(item, devmap_device_t, devices);
		
		if (device->namespace == namespace) {
			desc[pos].handle = device->handle;
			str_cpy(desc[pos].name, DEVMAP_NAME_MAXLEN, device->name);
			pos++;
		}
	}
	
	sysarg_t retval = async_data_read_finalize(callid, desc, size);
	
	free(desc);
	fibril_mutex_unlock(&devices_list_mutex);
	
	async_answer_0(iid, retval);
}

static void devmap_null_create(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&null_devices_mutex);
	
	unsigned int i;
	bool fnd = false;
	
	for (i = 0; i < NULL_DEVICES; i++) {
		if (null_devices[i] == NULL) {
			fnd = true;
			break;
		}
	}
	
	if (!fnd) {
		fibril_mutex_unlock(&null_devices_mutex);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	char null[DEVMAP_NAME_MAXLEN];
	snprintf(null, DEVMAP_NAME_MAXLEN, "%u", i);
	
	char *dev_name = str_dup(null);
	if (dev_name == NULL) {
		fibril_mutex_unlock(&null_devices_mutex);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	devmap_device_t *device =
	    (devmap_device_t *) malloc(sizeof(devmap_device_t));
	if (device == NULL) {
		fibril_mutex_unlock(&null_devices_mutex);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	fibril_mutex_lock(&devices_list_mutex);
	
	devmap_namespace_t *namespace = devmap_namespace_create("null");
	if (namespace == NULL) {
		fibril_mutex_lock(&devices_list_mutex);
		fibril_mutex_unlock(&null_devices_mutex);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&device->devices);
	link_initialize(&device->driver_devices);
	
	/* Get unique device handle */
	device->handle = devmap_create_handle();
	device->driver = NULL;
	
	devmap_namespace_addref(namespace, device);
	device->name = dev_name;
	
	/*
	 * Insert device into list of all devices and into null devices array.
	 * Insert device into a dummy list of null driver's devices so that it
	 * can be safely removed later.
	 */
	list_append(&device->devices, &devices_list);
	list_append(&device->driver_devices, &dummy_null_driver_devices);
	null_devices[i] = device;
	
	fibril_mutex_unlock(&devices_list_mutex);
	fibril_mutex_unlock(&null_devices_mutex);
	
	async_answer_1(iid, EOK, (sysarg_t) i);
}

static void devmap_null_destroy(ipc_callid_t iid, ipc_call_t *icall)
{
	sysarg_t i = IPC_GET_ARG1(*icall);
	if (i >= NULL_DEVICES) {
		async_answer_0(iid, ELIMIT);
		return;
	}
	
	fibril_mutex_lock(&null_devices_mutex);
	
	if (null_devices[i] == NULL) {
		fibril_mutex_unlock(&null_devices_mutex);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	fibril_mutex_lock(&devices_list_mutex);
	devmap_device_unregister_core(null_devices[i]);
	fibril_mutex_unlock(&devices_list_mutex);
	
	null_devices[i] = NULL;
	
	fibril_mutex_unlock(&null_devices_mutex);
	async_answer_0(iid, EOK);
}

/** Initialize device mapper.
 *
 *
 */
static bool devmap_init(void)
{
	fibril_mutex_lock(&null_devices_mutex);
	
	unsigned int i;
	for (i = 0; i < NULL_DEVICES; i++)
		null_devices[i] = NULL;
	
	fibril_mutex_unlock(&null_devices_mutex);
	
	return true;
}

/** Handle connection with device driver.
 *
 */
static void devmap_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	async_answer_0(iid, EOK);
	
	devmap_driver_t *driver = devmap_driver_register();
	if (driver == NULL)
		return;
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAP_DRIVER_UNREGISTER:
			if (NULL == driver)
				async_answer_0(callid, ENOENT);
			else
				async_answer_0(callid, EOK);
			break;
		case DEVMAP_DEVICE_REGISTER:
			/* Register one instance of device */
			devmap_device_register(callid, &call, driver);
			break;
		case DEVMAP_DEVICE_UNREGISTER:
			/* Remove instance of device identified by handler */
			devmap_device_unregister(callid, &call, driver);
			break;
		case DEVMAP_DEVICE_GET_HANDLE:
			devmap_device_get_handle(callid, &call);
			break;
		case DEVMAP_NAMESPACE_GET_HANDLE:
			devmap_namespace_get_handle(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
	
	if (driver != NULL) {
		/*
		 * Unregister the device driver and all its devices.
		 */
		devmap_driver_unregister(driver);
		driver = NULL;
	}
}

/** Handle connection with device client.
 *
 */
static void devmap_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAP_DEVICE_GET_HANDLE:
			devmap_device_get_handle(callid, &call);
			break;
		case DEVMAP_NAMESPACE_GET_HANDLE:
			devmap_namespace_get_handle(callid, &call);
			break;
		case DEVMAP_HANDLE_PROBE:
			devmap_handle_probe(callid, &call);
			break;
		case DEVMAP_NULL_CREATE:
			devmap_null_create(callid, &call);
			break;
		case DEVMAP_NULL_DESTROY:
			devmap_null_destroy(callid, &call);
			break;
		case DEVMAP_GET_NAMESPACE_COUNT:
			devmap_get_namespace_count(callid, &call);
			break;
		case DEVMAP_GET_DEVICE_COUNT:
			devmap_get_device_count(callid, &call);
			break;
		case DEVMAP_GET_NAMESPACES:
			devmap_get_namespaces(callid, &call);
			break;
		case DEVMAP_GET_DEVICES:
			devmap_get_devices(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}

/** Function for handling connections to devmap
 *
 */
static void devmap_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Select interface */
	switch ((sysarg_t) (IPC_GET_ARG1(*icall))) {
	case DEVMAP_DRIVER:
		devmap_connection_driver(iid, icall);
		break;
	case DEVMAP_CLIENT:
		devmap_connection_client(iid, icall);
		break;
	case DEVMAP_CONNECT_TO_DEVICE:
		/* Connect client to selected device */
		devmap_forward(iid, icall);
		break;
	default:
		/* No such interface */
		async_answer_0(iid, ENOENT);
	}
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf("%s: HelenOS Device Mapper\n", NAME);
	
	if (!devmap_init()) {
		printf("%s: Error while initializing service\n", NAME);
		return -1;
	}
	
	/* Set a handler of incomming connections */
	async_set_client_connection(devmap_connection);
	
	/* Register device mapper at naming service */
	if (service_register(SERVICE_DEVMAP) != EOK)
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
