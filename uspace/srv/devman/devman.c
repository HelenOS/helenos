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
/** @file Device Manager
 *
 * Locking order:
 *   (1) driver_t.driver_mutex
 *   (2) dev_tree_t.rwlock
 *
 * Synchronization:
 *    - device_tree.rwlock protects:
 *        - tree root, complete tree topology
 *        - complete contents of device and function nodes
 *    - dev_node_t.refcnt, fun_node_t.refcnt prevent nodes from
 *      being deallocated
 *    - find_xxx() functions increase reference count of returned object
 *    - find_xxx_no_lock() do not increase reference count
 *
 * TODO
 *    - Track all steady and transient device/function states
 *    - Check states, wait for steady state on certain operations
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io/log.h>
#include <ipc/driver.h>
#include <ipc/devman.h>
#include <loc.h>
#include <str_error.h>
#include <stdio.h>

#include "devman.h"

static fun_node_t *find_node_child(dev_tree_t *, fun_node_t *, const char *);

/* hash table operations */

static hash_index_t devices_hash(unsigned long key[])
{
	return key[0] % DEVICE_BUCKETS;
}

static int devman_devices_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	dev_node_t *dev = hash_table_get_instance(item, dev_node_t, devman_dev);
	return (dev->handle == (devman_handle_t) key[0]);
}

static int devman_functions_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	fun_node_t *fun = hash_table_get_instance(item, fun_node_t, devman_fun);
	return (fun->handle == (devman_handle_t) key[0]);
}

static int loc_functions_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	fun_node_t *fun = hash_table_get_instance(item, fun_node_t, loc_fun);
	return (fun->service_id == (service_id_t) key[0]);
}

static void devices_remove_callback(link_t *item)
{
}

static hash_table_operations_t devman_devices_ops = {
	.hash = devices_hash,
	.compare = devman_devices_compare,
	.remove_callback = devices_remove_callback
};

static hash_table_operations_t devman_functions_ops = {
	.hash = devices_hash,
	.compare = devman_functions_compare,
	.remove_callback = devices_remove_callback
};

static hash_table_operations_t loc_devices_ops = {
	.hash = devices_hash,
	.compare = loc_functions_compare,
	.remove_callback = devices_remove_callback
};

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
	list_prepend(&drv->drivers, &drivers_list->drivers);
	fibril_mutex_unlock(&drivers_list->drivers_mutex);

	log_msg(LVL_NOTE, "Driver `%s' was added to the list of available "
	    "drivers.", drv->name);
}

/** Read match id at the specified position of a string and set the position in
 * the string to the first character following the id.
 *
 * @param buf		The position in the input string.
 * @return		The match id.
 */
char *read_match_id(char **buf)
{
	char *res = NULL;
	size_t len = get_nonspace_len(*buf);
	
	if (len > 0) {
		res = malloc(len + 1);
		if (res != NULL) {
			str_ncpy(res, len + 1, *buf, len);
			*buf += len;
		}
	}
	
	return res;
}

/**
 * Read match ids and associated match scores from a string.
 *
 * Each match score in the string is followed by its match id.
 * The match ids and match scores are separated by whitespaces.
 * Neither match ids nor match scores can contain whitespaces.
 *
 * @param buf		The string from which the match ids are read.
 * @param ids		The list of match ids into which the match ids and
 *			scores are added.
 * @return		True if at least one match id and associated match score
 *			was successfully read, false otherwise.
 */
bool parse_match_ids(char *buf, match_id_list_t *ids)
{
	int score = 0;
	char *id = NULL;
	int ids_read = 0;
	
	while (true) {
		/* skip spaces */
		if (!skip_spaces(&buf))
			break;
		
		/* read score */
		score = strtoul(buf, &buf, 10);
		
		/* skip spaces */
		if (!skip_spaces(&buf))
			break;
		
		/* read id */
		id = read_match_id(&buf);
		if (NULL == id)
			break;
		
		/* create new match_id structure */
		match_id_t *mid = create_match_id();
		mid->id = id;
		mid->score = score;
		
		/* add it to the list */
		add_match_id(ids, mid);
		
		ids_read++;
	}
	
	return ids_read > 0;
}

/**
 * Read match ids and associated match scores from a file.
 *
 * Each match score in the file is followed by its match id.
 * The match ids and match scores are separated by whitespaces.
 * Neither match ids nor match scores can contain whitespaces.
 *
 * @param buf		The path to the file from which the match ids are read.
 * @param ids		The list of match ids into which the match ids and
 *			scores are added.
 * @return		True if at least one match id and associated match score
 *			was successfully read, false otherwise.
 */
bool read_match_ids(const char *conf_path, match_id_list_t *ids)
{
	log_msg(LVL_DEBUG, "read_match_ids(conf_path=\"%s\")", conf_path);
	
	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len = 0;
	
	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		log_msg(LVL_ERROR, "Unable to open `%s' for reading: %s.",
		    conf_path, str_error(fd));
		goto cleanup;
	}
	opened = true;
	
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (len == 0) {
		log_msg(LVL_ERROR, "Configuration file '%s' is empty.",
		    conf_path);
		goto cleanup;
	}
	
	buf = malloc(len + 1);
	if (buf == NULL) {
		log_msg(LVL_ERROR, "Memory allocation failed when parsing file "
		    "'%s'.", conf_path);
		goto cleanup;
	}
	
	ssize_t read_bytes = read_all(fd, buf, len);
	if (read_bytes <= 0) {
		log_msg(LVL_ERROR, "Unable to read file '%s' (%zd).", conf_path,
		    read_bytes);
		goto cleanup;
	}
	buf[read_bytes] = 0;
	
	suc = parse_match_ids(buf, ids);
	
cleanup:
	free(buf);
	
	if (opened)
		close(fd);
	
	return suc;
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
	log_msg(LVL_DEBUG, "get_driver_info(base_path=\"%s\", name=\"%s\")",
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
	struct stat s;
	if (stat(drv->binary_path, &s) == ENOENT) { /* FIXME!! */
		log_msg(LVL_ERROR, "Driver not found at path `%s'.",
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
	log_msg(LVL_DEBUG, "lookup_available_drivers(dir=\"%s\")", dir_path);
	
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

/** Create root device and function node in the device tree.
 *
 * @param tree	The device tree.
 * @return	True on success, false otherwise.
 */
bool create_root_nodes(dev_tree_t *tree)
{
	fun_node_t *fun;
	dev_node_t *dev;
	
	log_msg(LVL_DEBUG, "create_root_nodes()");
	
	fibril_rwlock_write_lock(&tree->rwlock);
	
	/*
	 * Create root function. This is a pseudo function to which
	 * the root device node is attached. It allows us to match
	 * the root device driver in a standard manner, i.e. against
	 * the parent function.
	 */
	
	fun = create_fun_node();
	if (fun == NULL) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		return false;
	}
	
	fun_add_ref(fun);
	insert_fun_node(tree, fun, str_dup(""), NULL);
	
	match_id_t *id = create_match_id();
	id->id = str_dup("root");
	id->score = 100;
	add_match_id(&fun->match_ids, id);
	tree->root_node = fun;
	
	/*
	 * Create root device node.
	 */
	dev = create_dev_node();
	if (dev == NULL) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		return false;
	}
	
	dev_add_ref(dev);
	insert_dev_node(tree, dev, fun);
	
	fibril_rwlock_write_unlock(&tree->rwlock);
	
	return dev != NULL;
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
	driver_t *best_drv = NULL, *drv = NULL;
	int best_score = 0, score = 0;
	
	fibril_mutex_lock(&drivers_list->drivers_mutex);
	
	list_foreach(drivers_list->drivers, link) {
		drv = list_get_instance(link, driver_t, drivers);
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
	log_msg(LVL_DEBUG, "attach_driver(dev=\"%s\",drv=\"%s\")",
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
	
	log_msg(LVL_DEBUG, "detach_driver(dev=\"%s\",drv=\"%s\")",
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
	int rc;

	assert(fibril_mutex_is_locked(&drv->driver_mutex));
	
	log_msg(LVL_DEBUG, "start_driver(drv=\"%s\")", drv->name);
	
	rc = task_spawnl(NULL, drv->binary_path, drv->binary_path, NULL);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Spawning driver `%s' (%s) failed: %s.",
		    drv->name, drv->binary_path, str_error(rc));
		return false;
	}
	
	drv->state = DRIVER_STARTING;
	return true;
}

/** Find device driver in the list of device drivers.
 *
 * @param drv_list	The list of device drivers.
 * @param drv_name	The name of the device driver which is searched.
 * @return		The device driver of the specified name, if it is in the
 *			list, NULL otherwise.
 */
driver_t *find_driver(driver_list_t *drv_list, const char *drv_name)
{
	driver_t *res = NULL;
	driver_t *drv = NULL;
	
	fibril_mutex_lock(&drv_list->drivers_mutex);
	
	list_foreach(drv_list->drivers, link) {
		drv = list_get_instance(link, driver_t, drivers);
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

	log_msg(LVL_DEBUG, "pass_devices_to_driver(driver=\"%s\")",
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

		log_msg(LVL_DEBUG, "pass_devices_to_driver: dev->refcnt=%d\n",
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
	log_msg(LVL_DEBUG, "Driver `%s' enters running state.", driver->name);
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
	log_msg(LVL_DEBUG, "initialize_running_driver(driver=\"%s\")",
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

	free_not_null(drv->name);
	free_not_null(drv->binary_path);

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

/** Create loc path and name for the function. */
void loc_register_tree_function(fun_node_t *fun, dev_tree_t *tree)
{
	char *loc_pathname = NULL;
	char *loc_name = NULL;
	
	assert(fibril_rwlock_is_locked(&tree->rwlock));
	
	asprintf(&loc_name, "%s", fun->pathname);
	if (loc_name == NULL)
		return;
	
	replace_char(loc_name, '/', LOC_SEPARATOR);
	
	asprintf(&loc_pathname, "%s/%s", LOC_DEVICE_NAMESPACE,
	    loc_name);
	if (loc_pathname == NULL) {
		free(loc_name);
		return;
	}
	
	loc_service_register_with_iface(loc_pathname,
	    &fun->service_id, DEVMAN_CONNECT_FROM_LOC);
	
	tree_add_loc_function(tree, fun);
	
	free(loc_name);
	free(loc_pathname);
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
	log_msg(LVL_DEBUG, "add_device(drv=\"%s\", dev=\"%s\")",
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
	sysarg_t rc = async_data_write_start(exch, dev->pfun->name,
	    str_size(dev->pfun->name) + 1);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		/* TODO handle error */
	}

	/* Wait for answer from the driver. */
	async_wait_for(req, &rc);

	switch(rc) {
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

	return;
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
		log_msg(LVL_ERROR, "No driver found for device `%s'.",
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

int driver_dev_remove(dev_tree_t *tree, dev_node_t *dev)
{
	async_exch_t *exch;
	sysarg_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	assert(dev != NULL);
	
	log_msg(LVL_DEBUG, "driver_dev_remove(%p)", dev);
	
	fibril_rwlock_read_lock(&tree->rwlock);
	drv = dev->drv;
	handle = dev->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_DEV_REMOVE, handle);
	async_exchange_end(exch);
	
	return retval;
}

int driver_dev_gone(dev_tree_t *tree, dev_node_t *dev)
{
	async_exch_t *exch;
	sysarg_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	assert(dev != NULL);
	
	log_msg(LVL_DEBUG, "driver_dev_gone(%p)", dev);
	
	fibril_rwlock_read_lock(&tree->rwlock);
	drv = dev->drv;
	handle = dev->handle;
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	exch = async_exchange_begin(drv->sess);
	retval = async_req_1_0(exch, DRIVER_DEV_GONE, handle);
	async_exchange_end(exch);
	
	return retval;
}

int driver_fun_online(dev_tree_t *tree, fun_node_t *fun)
{
	async_exch_t *exch;
	sysarg_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	log_msg(LVL_DEBUG, "driver_fun_online(%p)", fun);

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

int driver_fun_offline(dev_tree_t *tree, fun_node_t *fun)
{
	async_exch_t *exch;
	sysarg_t retval;
	driver_t *drv;
	devman_handle_t handle;
	
	log_msg(LVL_DEBUG, "driver_fun_offline(%p)", fun);

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

/** Initialize the device tree.
 *
 * Create root device node of the tree and assign driver to it.
 *
 * @param tree		The device tree.
 * @param drivers_list	the list of available drivers.
 * @return		True on success, false otherwise.
 */
bool init_device_tree(dev_tree_t *tree, driver_list_t *drivers_list)
{
	log_msg(LVL_DEBUG, "init_device_tree()");
	
	tree->current_handle = 0;
	
	hash_table_create(&tree->devman_devices, DEVICE_BUCKETS, 1,
	    &devman_devices_ops);
	hash_table_create(&tree->devman_functions, DEVICE_BUCKETS, 1,
	    &devman_functions_ops);
	hash_table_create(&tree->loc_functions, DEVICE_BUCKETS, 1,
	    &loc_devices_ops);
	
	fibril_rwlock_initialize(&tree->rwlock);
	
	/* Create root function and root device and add them to the device tree. */
	if (!create_root_nodes(tree))
		return false;
    
	/* Find suitable driver and start it. */
	dev_node_t *rdev = tree->root_node->child;
	dev_add_ref(rdev);
	int rc = assign_driver(rdev, drivers_list, tree);
	dev_del_ref(rdev);
	
	return rc;
}

/* Device nodes */

/** Create a new device node.
 *
 * @return		A device node structure.
 */
dev_node_t *create_dev_node(void)
{
	dev_node_t *dev;
	
	dev = calloc(1, sizeof(dev_node_t));
	if (dev == NULL)
		return NULL;
	
	atomic_set(&dev->refcnt, 0);
	list_initialize(&dev->functions);
	link_initialize(&dev->driver_devices);
	link_initialize(&dev->devman_dev);
	
	return dev;
}

/** Delete a device node.
 *
 * @param node		The device node structure.
 */
void delete_dev_node(dev_node_t *dev)
{
	assert(list_empty(&dev->functions));
	assert(dev->pfun == NULL);
	assert(dev->drv == NULL);
	
	free(dev);
}

/** Increase device node reference count.
 *
 * @param dev	Device node
 */
void dev_add_ref(dev_node_t *dev)
{
	atomic_inc(&dev->refcnt);
}

/** Decrease device node reference count.
 *
 * When the count drops to zero the device node is freed.
 *
 * @param dev	Device node
 */
void dev_del_ref(dev_node_t *dev)
{
	if (atomic_predec(&dev->refcnt) == 0)
		delete_dev_node(dev);
}


/** Find the device node structure of the device witch has the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
dev_node_t *find_dev_node_no_lock(dev_tree_t *tree, devman_handle_t handle)
{
	unsigned long key = handle;
	link_t *link;
	
	assert(fibril_rwlock_is_locked(&tree->rwlock));
	
	link = hash_table_find(&tree->devman_devices, &key);
	if (link == NULL)
		return NULL;
	
	return hash_table_get_instance(link, dev_node_t, devman_dev);
}

/** Find the device node structure of the device witch has the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
dev_node_t *find_dev_node(dev_tree_t *tree, devman_handle_t handle)
{
	dev_node_t *dev = NULL;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	dev = find_dev_node_no_lock(tree, handle);
	if (dev != NULL)
		dev_add_ref(dev);
	
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return dev;
}

/** Get list of device functions. */
int dev_get_functions(dev_tree_t *tree, dev_node_t *dev,
    devman_handle_t *hdl_buf, size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	assert(fibril_rwlock_is_locked(&tree->rwlock));

	buf_cnt = buf_size / sizeof(devman_handle_t);

	act_cnt = list_count(&dev->functions);
	*act_size = act_cnt * sizeof(devman_handle_t);

	if (buf_size % sizeof(devman_handle_t) != 0)
		return EINVAL;

	size_t pos = 0;
	list_foreach(dev->functions, item) {
		fun_node_t *fun =
		    list_get_instance(item, fun_node_t, dev_functions);

		if (pos < buf_cnt) {
			hdl_buf[pos] = fun->handle;
		}

		pos++;
	}

	return EOK;
}


/* Function nodes */

/** Create a new function node.
 *
 * @return		A function node structure.
 */
fun_node_t *create_fun_node(void)
{
	fun_node_t *fun;

	fun = calloc(1, sizeof(fun_node_t));
	if (fun == NULL)
		return NULL;
	
	fun->state = FUN_INIT;
	atomic_set(&fun->refcnt, 0);
	link_initialize(&fun->dev_functions);
	list_initialize(&fun->match_ids.ids);
	link_initialize(&fun->devman_fun);
	link_initialize(&fun->loc_fun);
	
	return fun;
}

/** Delete a function node.
 *
 * @param fun		The device node structure.
 */
void delete_fun_node(fun_node_t *fun)
{
	assert(fun->dev == NULL);
	assert(fun->child == NULL);
	
	clean_match_ids(&fun->match_ids);
	free_not_null(fun->name);
	free_not_null(fun->pathname);
	free(fun);
}

/** Increase function node reference count.
 *
 * @param fun	Function node
 */
void fun_add_ref(fun_node_t *fun)
{
	atomic_inc(&fun->refcnt);
}

/** Decrease function node reference count.
 *
 * When the count drops to zero the function node is freed.
 *
 * @param fun	Function node
 */
void fun_del_ref(fun_node_t *fun)
{
	if (atomic_predec(&fun->refcnt) == 0)
		delete_fun_node(fun);
}

/** Find the function node with the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the function.
 * @return		The function node.
 */
fun_node_t *find_fun_node_no_lock(dev_tree_t *tree, devman_handle_t handle)
{
	unsigned long key = handle;
	link_t *link;
	fun_node_t *fun;
	
	assert(fibril_rwlock_is_locked(&tree->rwlock));
	
	link = hash_table_find(&tree->devman_functions, &key);
	if (link == NULL)
		return NULL;
	
	fun = hash_table_get_instance(link, fun_node_t, devman_fun);
	
	return fun;
}

/** Find the function node with the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the function.
 * @return		The function node.
 */
fun_node_t *find_fun_node(dev_tree_t *tree, devman_handle_t handle)
{
	fun_node_t *fun = NULL;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	
	fun = find_fun_node_no_lock(tree, handle);
	if (fun != NULL)
		fun_add_ref(fun);
	
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return fun;
}

/** Create and set device's full path in device tree.
 *
 * @param tree		Device tree
 * @param node		The device's device node.
 * @param parent	The parent device node.
 * @return		True on success, false otherwise (insufficient
 *			resources etc.).
 */
static bool set_fun_path(dev_tree_t *tree, fun_node_t *fun, fun_node_t *parent)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	assert(fun->name != NULL);
	
	size_t pathsize = (str_size(fun->name) + 1);
	if (parent != NULL)
		pathsize += str_size(parent->pathname) + 1;
	
	fun->pathname = (char *) malloc(pathsize);
	if (fun->pathname == NULL) {
		log_msg(LVL_ERROR, "Failed to allocate device path.");
		return false;
	}
	
	if (parent != NULL) {
		str_cpy(fun->pathname, pathsize, parent->pathname);
		str_append(fun->pathname, pathsize, "/");
		str_append(fun->pathname, pathsize, fun->name);
	} else {
		str_cpy(fun->pathname, pathsize, fun->name);
	}
	
	return true;
}

/** Insert new device into device tree.
 *
 * @param tree		The device tree.
 * @param dev		The newly added device node.
 * @param pfun		The parent function node.
 *
 * @return		True on success, false otherwise (insufficient resources
 *			etc.).
 */
bool insert_dev_node(dev_tree_t *tree, dev_node_t *dev, fun_node_t *pfun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	log_msg(LVL_DEBUG, "insert_dev_node(dev=%p, pfun=%p [\"%s\"])",
	    dev, pfun, pfun->pathname);

	/* Add the node to the handle-to-node map. */
	dev->handle = ++tree->current_handle;
	unsigned long key = dev->handle;
	hash_table_insert(&tree->devman_devices, &key, &dev->devman_dev);

	/* Add the node to the list of its parent's children. */
	dev->pfun = pfun;
	pfun->child = dev;
	
	return true;
}

/** Remove device from device tree.
 *
 * @param tree		Device tree
 * @param dev		Device node
 */
void remove_dev_node(dev_tree_t *tree, dev_node_t *dev)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	log_msg(LVL_DEBUG, "remove_dev_node(dev=%p)", dev);
	
	/* Remove node from the handle-to-node map. */
	unsigned long key = dev->handle;
	hash_table_remove(&tree->devman_devices, &key, 1);
	
	/* Unlink from parent function. */
	dev->pfun->child = NULL;
	dev->pfun = NULL;
	
	dev->state = DEVICE_REMOVED;
}


/** Insert new function into device tree.
 *
 * @param tree		The device tree.
 * @param fun		The newly added function node. 
 * @param fun_name	The name of the newly added function.
 * @param dev		Owning device node.
 *
 * @return		True on success, false otherwise (insufficient resources
 *			etc.).
 */
bool insert_fun_node(dev_tree_t *tree, fun_node_t *fun, char *fun_name,
    dev_node_t *dev)
{
	fun_node_t *pfun;
	
	assert(fun_name != NULL);
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	/*
	 * The root function is a special case, it does not belong to any
	 * device so for the root function dev == NULL.
	 */
	pfun = (dev != NULL) ? dev->pfun : NULL;
	
	fun->name = fun_name;
	if (!set_fun_path(tree, fun, pfun)) {
		return false;
	}
	
	/* Add the node to the handle-to-node map. */
	fun->handle = ++tree->current_handle;
	unsigned long key = fun->handle;
	hash_table_insert(&tree->devman_functions, &key, &fun->devman_fun);

	/* Add the node to the list of its parent's children. */
	fun->dev = dev;
	if (dev != NULL)
		list_append(&fun->dev_functions, &dev->functions);
	
	return true;
}

/** Remove function from device tree.
 *
 * @param tree		Device tree
 * @param node		Function node to remove
 */
void remove_fun_node(dev_tree_t *tree, fun_node_t *fun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	/* Remove the node from the handle-to-node map. */
	unsigned long key = fun->handle;
	hash_table_remove(&tree->devman_functions, &key, 1);
	
	/* Remove the node from the list of its parent's children. */
	if (fun->dev != NULL)
		list_remove(&fun->dev_functions);
	
	fun->dev = NULL;
	fun->state = FUN_REMOVED;
}

/** Find function node with a specified path in the device tree.
 * 
 * @param path		The path of the function node in the device tree.
 * @param tree		The device tree.
 * @return		The function node if it is present in the tree, NULL
 *			otherwise.
 */
fun_node_t *find_fun_node_by_path(dev_tree_t *tree, char *path)
{
	assert(path != NULL);

	bool is_absolute = path[0] == '/';
	if (!is_absolute) {
		return NULL;
	}

	fibril_rwlock_read_lock(&tree->rwlock);
	
	fun_node_t *fun = tree->root_node;
	fun_add_ref(fun);
	/*
	 * Relative path to the function from its parent (but with '/' at the
	 * beginning)
	 */
	char *rel_path = path;
	char *next_path_elem = NULL;
	bool cont = (rel_path[1] != '\0');
	
	while (cont && fun != NULL) {
		next_path_elem  = get_path_elem_end(rel_path + 1);
		if (next_path_elem[0] == '/') {
			cont = true;
			next_path_elem[0] = 0;
		} else {
			cont = false;
		}
		
		fun_node_t *cfun = find_node_child(tree, fun, rel_path + 1);
		fun_del_ref(fun);
		fun = cfun;
		
		if (cont) {
			/* Restore the original path. */
			next_path_elem[0] = '/';
		}
		rel_path = next_path_elem;
	}
	
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return fun;
}

/** Find function with a specified name belonging to given device.
 *
 * Device tree rwlock should be held at least for reading.
 *
 * @param tree Device tree
 * @param dev Device the function belongs to.
 * @param name Function name (not path).
 * @return Function node.
 * @retval NULL No function with given name.
 */
fun_node_t *find_fun_node_in_device(dev_tree_t *tree, dev_node_t *dev,
    const char *name)
{
	assert(name != NULL);
	assert(fibril_rwlock_is_locked(&tree->rwlock));

	fun_node_t *fun;

	list_foreach(dev->functions, link) {
		fun = list_get_instance(link, fun_node_t, dev_functions);

		if (str_cmp(name, fun->name) == 0) {
			fun_add_ref(fun);
			return fun;
		}
	}

	return NULL;
}

/** Find child function node with a specified name.
 *
 * Device tree rwlock should be held at least for reading.
 *
 * @param tree		Device tree
 * @param parent	The parent function node.
 * @param name		The name of the child function.
 * @return		The child function node.
 */
static fun_node_t *find_node_child(dev_tree_t *tree, fun_node_t *pfun,
    const char *name)
{
	return find_fun_node_in_device(tree, pfun->child, name);
}

/* loc devices */

fun_node_t *find_loc_tree_function(dev_tree_t *tree, service_id_t service_id)
{
	fun_node_t *fun = NULL;
	link_t *link;
	unsigned long key = (unsigned long) service_id;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	link = hash_table_find(&tree->loc_functions, &key);
	if (link != NULL) {
		fun = hash_table_get_instance(link, fun_node_t, loc_fun);
		fun_add_ref(fun);
	}
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return fun;
}

void tree_add_loc_function(dev_tree_t *tree, fun_node_t *fun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	unsigned long key = (unsigned long) fun->service_id;
	hash_table_insert(&tree->loc_functions, &key, &fun->loc_fun);
}

/** @}
 */
