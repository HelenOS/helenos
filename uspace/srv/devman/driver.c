/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup devman
 * @{
 */

#include <dirent.h>
#include <errno.h>
#include <io/log.h>
#include <vfs/vfs.h>
#include <loc.h>
#include <str_error.h>
#include <stdio.h>
#include <task.h>

#include "dev.h"
#include "devman.h"
#include "driver.h"
#include "match.h"

/**
 * Initialize the list of device driver's.
 *
 * @param drv_list the list of device driver's.
 *
 */
void init_driver_list(driver_list_t *drv_list)
{
	assert(drv_list != NULL);
	
	list_initialize(&drv_list->drivers);
	fibril_mutex_initialize(&drv_list->drivers_mutex);
	drv_list->next_handle = 1;
}

/** Allocate and initialize a new driver structure.
 *
 * @return	Driver structure.
 */
driver_t *create_driver(void)
{
	driver_t *res = malloc(sizeof(driver_t));
	if (res != NULL)
		init_driver(res);
	return res;
}

/** Add a driver to the list of drivers.
 *
 * @param drivers_list	List of drivers.
 * @param drv		Driver structure.
 */
void add_driver(driver_list_t *drivers_list, driver_t *drv)
{
	fibril_mutex_lock(&drivers_list->drivers_mutex);
	list_append(&drv->drivers, &drivers_list->drivers);
	drv->handle = drivers_list->next_handle++;
	fibril_mutex_unlock(&drivers_list->drivers_mutex);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Driver `%s' was added to the list of available "
	    "drivers.", drv->name);
}

/**
 * Get information about a driver.
 *
 * Each driver has its own directory in the base directory.
 * The name of the driver's directory is the same as the name of the driver.
 * The driver's directory contains driver's binary (named as the driver without
 * extension) and the configuration file with match ids for device-to-driver
 *  matching (named as the driver with a special extension).
 *
 * This function searches for the driver's directory and containing
 * configuration files. If all the files needed are found, they are parsed and
 * the information about the driver is stored in the driver's structure.
 *
 * @param base_path	The base directory, in which we look for driver's
 *			subdirectory.
 * @param name		The name of the driver.
 * @param drv		The driver structure to fill information in.
 *
 * @return		True on success, false otherwise.
 */
bool get_driver_info(const char *base_path, const char *name, driver_t *drv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "get_driver_info(base_path=\"%s\", name=\"%s\")",
	    base_path, name);
	
	assert(base_path != NULL && name != NULL && drv != NULL);
	
	bool suc = false;
	char *match_path = NULL;
	size_t name_size = 0;
	
	/* Read the list of match ids from the driver's configuration file. */
	match_path = get_abs_path(base_path, name, MATCH_EXT);
	if (match_path == NULL)
		goto cleanup;
	
	if (!read_match_ids(match_path, &drv->match_ids))
		goto cleanup;
	
	/* Allocate and fill driver's name. */
	name_size = str_size(name) + 1;
	drv->name = malloc(name_size);
	if (drv->name == NULL)
		goto cleanup;
	str_cpy(drv->name, name_size, name);
	
	/* Initialize path with driver's binary. */
	drv->binary_path = get_abs_path(base_path, name, "");
	if (drv->binary_path == NULL)
		goto cleanup;
	
	/* Check whether the driver's binary exists. */
	vfs_stat_t s;
	if (vfs_stat_path(drv->binary_path, &s) != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Driver not found at path `%s'.",
		    drv->binary_path);
		goto cleanup;
	}
	
	suc = true;
	
cleanup:
	if (!suc) {
		free(drv->binary_path);
		free(drv->name);
		/* Set the driver structure to the default state. */
		init_driver(drv);
	}
	
	free(match_path);
	
	return suc;
}

/** Lookup drivers in the directory.
 *
 * @param drivers_list	The list of available drivers.
 * @param dir_path	The path to the directory where we search for drivers.
 * @return		Number of drivers which were found.
 */
int lookup_available_drivers(driver_list_t *drivers_list, const char *dir_path)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "lookup_available_drivers(dir=\"%s\")", dir_path);
	
	int drv_cnt = 0;
	DIR *dir = NULL;
	struct dirent *diren;

	dir = opendir(dir_path);
	
	if (dir != NULL) {
		driver_t *drv = create_driver();
		while ((diren = readdir(dir))) {
			if (get_driver_info(dir_path, diren->d_name, drv)) {
				add_driver(drivers_list, drv);
				drv_cnt++;
				drv = create_driver();
			}
		}
		delete_driver(drv);
		closedir(dir);
	}
	
	return drv_cnt;
}

/** Lookup the best matching driver for the specified device in the list of
 * drivers.
 *
 * A match between a device and a driver is found if one of the driver's match
 * ids match one of the device's match ids. The score of the match is the
 * product of the driver's and device's score associated with the matching id.
 * The best matching driver for a device is the driver with the highest score
 * of the match between the device and the driver.
 *
 * @param drivers_list	The list of drivers, where we look for the driver
 *			suitable for handling the device.
 * @param node		The device node structure of the device.
 * @return		The best matching driver or NULL if no matching driver
 *			is found.
 */
driver_t *find_best_match_driver(driver_list_t *drivers_list, dev_node_t *node)
{
	driver_t *best_drv = NULL;
	int best_score = 0, score = 0;
	
	fibril_mutex_lock(&drivers_list->drivers_mutex);
	
	list_foreach(drivers_list->drivers, drivers, driver_t, drv) {
		score = get_match_score(drv, node);
		if (score > best_score) {
			best_score = score;
			best_drv = drv;
		}
	}
	
	fibril_mutex_unlock(&drivers_list->drivers_mutex);
	
	return best_drv;
}

/** Assign a driver to a device.
 *
 * @param tree		Device tree
 * @param node		The device's node in the device tree.
 * @param drv		The driver.
 */
void attach_driver(dev_tree_t *tree, dev_node_t *dev, driver_t *drv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "attach_driver(dev=\"%s\",drv=\"%s\")",
	    dev->pfun->pathname, drv->name);
	
	fibril_mutex_lock(&drv->driver_mutex);
	fibril_rwlock_write_lock(&tree->rwlock);
	
	dev->drv = drv;
	list_append(&dev->driver_devices, &drv->devices);
	
	fibril_rwlock_write_unlock(&tree->rwlock);
	fibril_mutex_unlock(&drv->driver_mutex);
}

/** Detach driver from device.
 *
 * @param tree		Device tree
 * @param node		The device's node in the device tree.
 * @param drv		The driver.
 */
void detach_driver(dev_tree_t *tree, dev_node_t *dev)
{
	driver_t *drv = dev->drv;
	
	assert(drv != NULL);
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "detach_driver(dev=\"%s\",drv=\"%s\")",
	    dev->pfun->pathname, drv->name);
	
	fibril_mutex_lock(&drv->driver_mutex);
	fibril_rwlock_write_lock(&tree->rwlock);
	
	dev->drv = NULL;
	list_remove(&dev->driver_devices);
	
	fibril_rwlock_write_unlock(&tree->rwlock);
	fibril_mutex_unlock(&drv->driver_mutex);
}

/** Start a driver
 *
 * @param drv		The driver's structure.
 * @return		True if the driver's task is successfully spawned, false
 *			otherwise.
 */
bool start_driver(driver_t *drv)
{
	errno_t rc;

	assert(fibril_mutex_is_locked(&drv->driver_mutex));
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "start_driver(drv=\"%s\")", drv->name);
	
	rc = task_spawnl(NULL, NULL, drv->binary_path, drv->binary_path, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Spawning driver `%s' (%s) failed: %s.",
		    drv->name, drv->binary_path, str_error(rc));
		return false;
	}
	
	drv->state = DRIVER_STARTING;
	return true;
}

/** Stop a driver
 *
 * @param drv		The driver's structure.
 * @return		True if the driver's task is successfully spawned, false
 *			otherwise.
 */
errno_t stop_driver(driver_t *drv)
{
	async_exch_t *exch;
	errno_t retval;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "stop_driver(drv=\"%s\")", drv->name);

	exch = async_exchange_begin(drv->sess);
	retval = async_req_0_0(exch, DRIVER_STOP);
	loc_exchange_end(exch);
	
	if (retval != EOK)
		return retval;
	
	drv->state = DRIVER_NOT_STARTED;
	async_hangup(drv->sess);
	drv->sess = NULL;
	return EOK;
}

/** Find device driver by handle.
 *
 * @param drv_list	The list of device drivers
 * @param handle	Driver handle
 * @return		The device driver, if it is in the list,
 *			NULL otherwise.
 */
driver_t *driver_find(driver_list_t *drv_list, devman_handle_t handle)
{
	driver_t *res = NULL;
	
	fibril_mutex_lock(&drv_list->drivers_mutex);
	
	list_foreach(drv_list->drivers, drivers, driver_t, drv) {
		if (drv->handle == handle) {
			res = drv;
			break;
		}
	}
	
	fibril_mutex_unlock(&drv_list->drivers_mutex);
	
	return res;
}


/** Find device driver by name.
 *
 * @param drv_list	The list of device drivers.
 * @param drv_name	The name of the device driver which is searched.
 * @return		The device driver of the specified name, if it is in the
 *			list, NULL otherwise.
 */
driver_t *driver_find_by_name(driver_list_t *drv_list, const char *drv_name)
{
	driver_t *res = NULL;
	
	fibril_mutex_lock(&drv_list->drivers_mutex);
	
	list_foreach(drv_list->drivers, drivers, driver_t, drv) {
		if (str_cmp(drv->name, drv_name) == 0) {
			res = drv;
			break;
		}
	}
	
	fibril_mutex_unlock(&drv_list->drivers_mutex);
	
	return res;
}

/** Notify driver about the devices to which it was assigned.
 *
 * @param driver	The driver to which the devices are passed.
 */
static void pass_devices_to_driver(driver_t *driver, dev_tree_t *tree)
{
	dev_node_t *dev;
	link_t *link;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "pass_devices_to_driver(driver=\"%s\")",
	    driver->name);

	fibril_mutex_lock(&driver->driver_mutex);

	/*
	 * Go through devices list as long as there is some device
	 * that has not been passed to the driver.
	 */
	link = driver->devices.head.next;
	while (link != &driver->devices.head) {
		dev = list_get_instance(link, dev_node_t, driver_devices);
		fibril_rwlock_write_lock(&tree->rwlock);
		
		if (dev->passed_to_driver) {
			fibril_rwlock_write_unlock(&tree->rwlock);
			link = link->next;
			continue;
		}

		log_msg(LOG_DEFAULT, LVL_DEBUG, "pass_devices_to_driver: dev->refcnt=%d\n",
		    (int)atomic_get(&dev->refcnt));
		dev_add_ref(dev);

		/*
		 * Unlock to avoid deadlock when adding device
		 * handled by itself.
		 */
		fibril_mutex_unlock(&driver->driver_mutex);
		fibril_rwlock_write_unlock(&tree->rwlock);

		add_device(driver, dev, tree);

		dev_del_ref(dev);

		/*
		 * Lock again as we will work with driver's
		 * structure.
		 */
		fibril_mutex_lock(&driver->driver_mutex);

		/*
		 * Restart the cycle to go through all devices again.
		 */
		link = driver->devices.head.next;
	}

	/*
	 * Once we passed all devices to the driver, we need to mark the
	 * driver as running.
	 * It is vital to do it here and inside critical section.
	 *
	 * If we would change the state earlier, other devices added to
	 * the driver would be added to the device list and started
	 * immediately and possibly started here as well.
	 */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "Driver `%s' enters running state.", driver->name);
	driver->state = DRIVER_RUNNING;

	fibril_mutex_unlock(&driver->driver_mutex);
}

/** Finish the initialization of a driver after it has succesfully started
 * and after it has registered itself by the device manager.
 *
 * Pass devices formerly matched to the driver to the driver and remember the
 * driver is running and fully functional now.
 *
 * @param driver	The driver which registered itself as running by the
 *			device manager.
 */
void initialize_running_driver(driver_t *driver, dev_tree_t *tree)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "initialize_running_driver(driver=\"%s\")",
	    driver->name);
	
	/*
	 * Pass devices which have been already assigned to the driver to the
	 * driver.
	 */
	pass_devices_to_driver(driver, tree);
}

/** Initialize device driver structure.
 *
 * @param drv		The device driver structure.
 */
void init_driver(driver_t *drv)
{
	assert(drv != NULL);

	memset(drv, 0, sizeof(driver_t));
	list_initialize(&drv->match_ids.ids);
	list_initialize(&drv->devices);
	fibril_mutex_initialize(&drv->driver_mutex);
	drv->sess = NULL;
}

/** Device driver structure clean-up.
 *
 * @param drv		The device driver structure.
 */
void clean_driver(driver_t *drv)
{
	assert(drv != NULL);

	free(drv->name);
	free(drv->binary_path);

	clean_match_ids(&drv->match_ids);

	init_driver(drv);
}

/** Delete device driver structure.
 *
 * @param drv		The device driver structure.
 */
void delete_driver(driver_t *drv)
{
	assert(drv != NULL);
	
	clean_driver(drv);
	free(drv);
}

/** Find suitable driver for a device and assign the driver to it.
 *
 * @param node		The device node of the device in the device tree.
 * @param drivers_list	The list of available drivers.
 * @return		True if the suitable driver is found and
 *			successfully assigned to the device, false otherwise.
 */
bool assign_driver(dev_node_t *dev, driver_list_t *drivers_list,
    dev_tree_t *tree)
{
	assert(dev != NULL);
	assert(drivers_list != NULL);
	assert(tree != NULL);
	
	/*
	 * Find the driver which is the most suitable for handling this device.
	 */
	driver_t *drv = find_best_match_driver(drivers_list, dev);
	if (drv == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "No driver found for device `%s'.",
		    dev->pfun->pathname);
		return false;
	}
	
	/* Attach the driver to the device. */
	attach_driver(tree, dev, drv);
	
	fibril_mutex_lock(&drv->driver_mutex);
	if (drv->state == DRIVER_NOT_STARTED) {
		/* Start the driver. */
		start_driver(drv);
	}
	bool is_running = drv->state == DRIVER_RUNNING;
	fibril_mutex_unlock(&drv->driver_mutex);

	/* Notify the driver about the new device. */
	if (is_running)
		add_device(drv, dev, tree);
	
	fibril_mutex_lock(&drv->driver_mutex);
	fibril_mutex_unlock(&drv->driver_mutex);

	fibril_rwlock_write_lock(&tree->rwlock);
	if (dev->pfun != NULL) {
		dev->pfun->state = FUN_ON_LINE;
	}
	fibril_rwlock_write_unlock(&tree->rwlock);
	return true;
}

/** Pass a device to running driver.
 *
 * @param drv		The driver's structure.
 * @param node		The device's node in the device tree.
 */
void add_device(driver_t *drv, dev_node_t *dev, dev_tree_t *tree)
{
	/*
	 * We do not expect to have driver's mutex locked as we do not
	 * access any structures that would affect driver_t.
	 */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "add_device(drv=\"%s\", dev=\"%s\")",
	    drv->name, dev->pfun->name);
	
	/* Send the device to the driver. */
	devman_handle_t parent_handle;
	if (dev->pfun) {
		parent_handle = dev->pfun->handle;
	} else {
		parent_handle = 0;
	}
	
	async_exch_t *exch = async_exchange_begin(drv->sess);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DRIVER_DEV_ADD, dev->handle,
	    parent_handle, &answer);
	
	/* Send the device name to the driver. */
	errno_t rc = async_data_write_start(exch, dev->pfun->name,
	    str_size(dev->pfun->name) + 1);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
	} else {
		/* Wait for answer from the driver. */
		async_wait_for(req, &rc);
	}

	switch (rc) {
	case EOK:
		dev->state = DEVICE_USABLE;
		break;
	case ENOENT:
		dev->state = DEVICE_NOT_PRESENT;
		break;
	default:
		dev->state = DEVICE_INVALID;
		break;
	}
	
	dev->passed_to_driver = true;
}

errno_t driver_dev_remove(dev_tree_t *tree, dev_node_t *dev)
{
	async_exch_t *exch;
	errno_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	assert(dev != NULL);
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "driver_dev_remove(%p)", dev);
	
	fibril_rwlock_read_lock(&tree->rwlock);
	drv = dev->drv;
	handle = dev->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_DEV_REMOVE, handle);
	async_exchange_end(exch);
	
	return retval;
}

errno_t driver_dev_gone(dev_tree_t *tree, dev_node_t *dev)
{
	async_exch_t *exch;
	errno_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	assert(dev != NULL);
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "driver_dev_gone(%p)", dev);
	
	fibril_rwlock_read_lock(&tree->rwlock);
	drv = dev->drv;
	handle = dev->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_DEV_GONE, handle);
	async_exchange_end(exch);
	
	return retval;
}

errno_t driver_fun_online(dev_tree_t *tree, fun_node_t *fun)
{
	async_exch_t *exch;
	errno_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "driver_fun_online(%p)", fun);

	fibril_rwlock_read_lock(&tree->rwlock);
	
	if (fun->dev == NULL) {
		/* XXX root function? */
		fibril_rwlock_read_unlock(&tree->rwlock);
		return EINVAL;
	}
	
	drv = fun->dev->drv;
	handle = fun->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_FUN_ONLINE, handle);
	loc_exchange_end(exch);
	
	return retval;
}

errno_t driver_fun_offline(dev_tree_t *tree, fun_node_t *fun)
{
	async_exch_t *exch;
	errno_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "driver_fun_offline(%p)", fun);

	fibril_rwlock_read_lock(&tree->rwlock);
	if (fun->dev == NULL) {
		/* XXX root function? */
		fibril_rwlock_read_unlock(&tree->rwlock);
		return EINVAL;
	}
	
	drv = fun->dev->drv;
	handle = fun->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_FUN_OFFLINE, handle);
	loc_exchange_end(exch);
	
	return retval;

}

/** Get list of registered drivers. */
errno_t driver_get_list(driver_list_t *driver_list, devman_handle_t *hdl_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&driver_list->drivers_mutex);

	buf_cnt = buf_size / sizeof(devman_handle_t);

	act_cnt = list_count(&driver_list->drivers);
	*act_size = act_cnt * sizeof(devman_handle_t);

	if (buf_size % sizeof(devman_handle_t) != 0) {
		fibril_mutex_unlock(&driver_list->drivers_mutex);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(driver_list->drivers, drivers, driver_t, drv) {
		if (pos < buf_cnt) {
			hdl_buf[pos] = drv->handle;
		}

		pos++;
	}

	fibril_mutex_unlock(&driver_list->drivers_mutex);
	return EOK;
}

/** Get list of device functions. */
errno_t driver_get_devices(driver_t *driver, devman_handle_t *hdl_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&driver->driver_mutex);

	buf_cnt = buf_size / sizeof(devman_handle_t);

	act_cnt = list_count(&driver->devices);
	*act_size = act_cnt * sizeof(devman_handle_t);

	if (buf_size % sizeof(devman_handle_t) != 0) {
		fibril_mutex_unlock(&driver->driver_mutex);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(driver->devices, driver_devices, dev_node_t, dev) {
		if (pos < buf_cnt) {
			hdl_buf[pos] = dev->handle;
		}

		pos++;
	}

	fibril_mutex_unlock(&driver->driver_mutex);
	return EOK;
}

/** @}
 */
