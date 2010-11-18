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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ipc/driver.h>
#include <ipc/devman.h>
#include <devmap.h>
#include <str_error.h>

#include "devman.h"

/* hash table operations */

static hash_index_t devices_hash(unsigned long key[])
{
	return key[0] % DEVICE_BUCKETS;
}

static int devman_devices_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	node_t *dev = hash_table_get_instance(item, node_t, devman_link);
	return (dev->handle == (devman_handle_t) key[0]);
}

static int devmap_devices_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	node_t *dev = hash_table_get_instance(item, node_t, devmap_link);
	return (dev->devmap_handle == (devmap_handle_t) key[0]);
}

static void devices_remove_callback(link_t *item)
{
}

static hash_table_operations_t devman_devices_ops = {
	.hash = devices_hash,
	.compare = devman_devices_compare,
	.remove_callback = devices_remove_callback
};

static hash_table_operations_t devmap_devices_ops = {
	.hash = devices_hash,
	.compare = devmap_devices_compare,
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

	printf(NAME": the '%s' driver was added to the list of available "
	    "drivers.\n", drv->name);
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
	printf(NAME ": read_match_ids conf_path = %s.\n", conf_path);
	
	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len = 0;
	
	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		printf(NAME ": unable to open %s\n", conf_path);
		goto cleanup;
	}
	opened = true;
	
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (len == 0) {
		printf(NAME ": configuration file '%s' is empty.\n", conf_path);
		goto cleanup;
	}
	
	buf = malloc(len + 1);
	if (buf == NULL) {
		printf(NAME ": memory allocation failed when parsing file "
		    "'%s'.\n", conf_path);
		goto cleanup;
	}
	
	if (read(fd, buf, len) <= 0) {
		printf(NAME ": unable to read file '%s'.\n", conf_path);
		goto cleanup;
	}
	buf[len] = 0;
	
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
	printf(NAME ": get_driver_info base_path = %s, name = %s.\n",
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
		printf(NAME ": driver not found at path %s.", drv->binary_path);
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
	printf(NAME ": lookup_available_drivers, dir = %s \n", dir_path);
	
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

/** Create root device node in the device tree.
 *
 * @param tree	The device tree.
 * @return	True on success, false otherwise.
 */
bool create_root_node(dev_tree_t *tree)
{
	node_t *node;

	printf(NAME ": create_root_node\n");

	node = create_dev_node();
	if (node != NULL) {
		insert_dev_node(tree, node, clone_string(""), NULL);
		match_id_t *id = create_match_id();
		id->id = clone_string("root");
		id->score = 100;
		add_match_id(&node->match_ids, id);
		tree->root_node = node;
	}

	return node != NULL;
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
driver_t *find_best_match_driver(driver_list_t *drivers_list, node_t *node)
{
	driver_t *best_drv = NULL, *drv = NULL;
	int best_score = 0, score = 0;
	
	fibril_mutex_lock(&drivers_list->drivers_mutex);
	
	link_t *link = drivers_list->drivers.next;
	while (link != &drivers_list->drivers) {
		drv = list_get_instance(link, driver_t, drivers);
		score = get_match_score(drv, node);
		if (score > best_score) {
			best_score = score;
			best_drv = drv;
		}
		link = link->next;
	}
	
	fibril_mutex_unlock(&drivers_list->drivers_mutex);
	
	return best_drv;
}

/** Assign a driver to a device.
 *
 * @param node		The device's node in the device tree.
 * @param drv		The driver.
 */
void attach_driver(node_t *node, driver_t *drv)
{
	printf(NAME ": attach_driver %s to device %s\n",
	    drv->name, node->pathname);
	
	fibril_mutex_lock(&drv->driver_mutex);
	
	node->drv = drv;
	list_append(&node->driver_devices, &drv->devices);
	
	fibril_mutex_unlock(&drv->driver_mutex);
}

/** Start a driver
 *
 * The driver's mutex is assumed to be locked.
 *
 * @param drv		The driver's structure.
 * @return		True if the driver's task is successfully spawned, false
 *			otherwise.
 */
bool start_driver(driver_t *drv)
{
	int rc;

	printf(NAME ": start_driver '%s'\n", drv->name);
	
	rc = task_spawnl(NULL, drv->binary_path, drv->binary_path, NULL);
	if (rc != EOK) {
		printf(NAME ": error spawning %s (%s)\n",
		    drv->name, str_error(rc));
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
	link_t *link;
	
	fibril_mutex_lock(&drv_list->drivers_mutex);
	
	link = drv_list->drivers.next;
	while (link != &drv_list->drivers) {
		drv = list_get_instance(link, driver_t, drivers);
		if (str_cmp(drv->name, drv_name) == 0) {
			res = drv;
			break;
		}

		link = link->next;
	}
	
	fibril_mutex_unlock(&drv_list->drivers_mutex);
	
	return res;
}

/** Remember the driver's phone.
 *
 * @param driver	The driver.
 * @param phone		The phone to the driver.
 */
void set_driver_phone(driver_t *driver, ipcarg_t phone)
{
	fibril_mutex_lock(&driver->driver_mutex);
	assert(driver->state == DRIVER_STARTING);
	driver->phone = phone;
	fibril_mutex_unlock(&driver->driver_mutex);
}

/** Notify driver about the devices to which it was assigned.
 *
 * The driver's mutex must be locked.
 *
 * @param driver	The driver to which the devices are passed.
 */
static void pass_devices_to_driver(driver_t *driver, dev_tree_t *tree)
{
	node_t *dev;
	link_t *link;
	int phone;

	printf(NAME ": pass_devices_to_driver\n");

	phone = ipc_connect_me_to(driver->phone, DRIVER_DEVMAN, 0, 0);
	if (phone > 0) {
		
		link = driver->devices.next;
		while (link != &driver->devices) {
			dev = list_get_instance(link, node_t, driver_devices);
			add_device(phone, driver, dev, tree);
			link = link->next;
		}
		
		ipc_hangup(phone);
	}
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
	printf(NAME ": initialize_running_driver\n");
	fibril_mutex_lock(&driver->driver_mutex);
	
	/*
	 * Pass devices which have been already assigned to the driver to the
	 * driver.
	 */
	pass_devices_to_driver(driver, tree);
	
	/* Change driver's state to running. */
	driver->state = DRIVER_RUNNING;
	
	fibril_mutex_unlock(&driver->driver_mutex);
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

/** Create devmap path and name for the device. */
static void devmap_register_tree_device(node_t *node, dev_tree_t *tree)
{
	char *devmap_pathname = NULL;
	char *devmap_name = NULL;
	
	asprintf(&devmap_name, "%s", node->pathname);
	if (devmap_name == NULL)
		return;
	
	replace_char(devmap_name, '/', DEVMAP_SEPARATOR);
	
	asprintf(&devmap_pathname, "%s/%s", DEVMAP_DEVICE_NAMESPACE,
	    devmap_name);
	if (devmap_pathname == NULL) {
		free(devmap_name);
		return;
	}
	
	devmap_device_register(devmap_pathname, &node->devmap_handle);
	
	tree_add_devmap_device(tree, node);
	
	free(devmap_name);
	free(devmap_pathname);
}


/** Pass a device to running driver.
 *
 * @param drv		The driver's structure.
 * @param node		The device's node in the device tree.
 */
void add_device(int phone, driver_t *drv, node_t *node, dev_tree_t *tree)
{
	printf(NAME ": add_device\n");
	
	ipcarg_t rc;
	ipc_call_t answer;
	
	/* Send the device to the driver. */
	aid_t req = async_send_1(phone, DRIVER_ADD_DEVICE, node->handle,
	    &answer);
	
	/* Send the device's name to the driver. */
	rc = async_data_write_start(phone, node->name,
	    str_size(node->name) + 1);
	if (rc != EOK) {
		/* TODO handle error */
	}
	
	/* Wait for answer from the driver. */
	async_wait_for(req, &rc);
	switch(rc) {
	case EOK:
		node->state = DEVICE_USABLE;
		devmap_register_tree_device(node, tree);
		break;
	case ENOENT:
		node->state = DEVICE_NOT_PRESENT;
		break;
	default:
		node->state = DEVICE_INVALID;
	}
	
	return;
}

/** Find suitable driver for a device and assign the driver to it.
 *
 * @param node		The device node of the device in the device tree.
 * @param drivers_list	The list of available drivers.
 * @return		True if the suitable driver is found and
 *			successfully assigned to the device, false otherwise.
 */
bool assign_driver(node_t *node, driver_list_t *drivers_list, dev_tree_t *tree)
{
	/*
	 * Find the driver which is the most suitable for handling this device.
	 */
	driver_t *drv = find_best_match_driver(drivers_list, node);
	if (drv == NULL) {
		printf(NAME ": no driver found for device '%s'.\n",
		    node->pathname);
		return false;
	}
	
	/* Attach the driver to the device. */
	attach_driver(node, drv);
	
	if (drv->state == DRIVER_NOT_STARTED) {
		/* Start the driver. */
		start_driver(drv);
	}
	
	if (drv->state == DRIVER_RUNNING) {
		/* Notify the driver about the new device. */
		int phone = ipc_connect_me_to(drv->phone, DRIVER_DEVMAN, 0, 0);
		if (phone > 0) {
			add_device(phone, drv, node, tree);
			ipc_hangup(phone);
		}
	}
	
	return true;
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
	printf(NAME ": init_device_tree.\n");
	
	tree->current_handle = 0;
	
	hash_table_create(&tree->devman_devices, DEVICE_BUCKETS, 1,
	    &devman_devices_ops);
	hash_table_create(&tree->devmap_devices, DEVICE_BUCKETS, 1,
	    &devmap_devices_ops);
	
	fibril_rwlock_initialize(&tree->rwlock);
	
	/* Create root node and add it to the device tree. */
	if (!create_root_node(tree))
		return false;

	/* Find suitable driver and start it. */
	return assign_driver(tree->root_node, drivers_list, tree);
}

/* Device nodes */

/** Create a new device node.
 *
 * @return		A device node structure.
 */
node_t *create_dev_node(void)
{
	node_t *res = malloc(sizeof(node_t));
	
	if (res != NULL) {
		memset(res, 0, sizeof(node_t));
		list_initialize(&res->children);
		list_initialize(&res->match_ids.ids);
		list_initialize(&res->classes);
	}
	
	return res;
}

/** Delete a device node.
 *
 * @param node		The device node structure.
 */
void delete_dev_node(node_t *node)
{
	assert(list_empty(&node->children));
	assert(node->parent == NULL);
	assert(node->drv == NULL);
	
	clean_match_ids(&node->match_ids);
	free_not_null(node->name);
	free_not_null(node->pathname);
	free(node);
}

/** Find the device node structure of the device witch has the specified handle.
 *
 * Device tree's rwlock should be held at least for reading.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
node_t *find_dev_node_no_lock(dev_tree_t *tree, devman_handle_t handle)
{
	unsigned long key = handle;
	link_t *link = hash_table_find(&tree->devman_devices, &key);
	return hash_table_get_instance(link, node_t, devman_link);
}

/** Find the device node structure of the device witch has the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
node_t *find_dev_node(dev_tree_t *tree, devman_handle_t handle)
{
	node_t *node = NULL;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	node = find_dev_node_no_lock(tree, handle);
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return node;
}


/** Create and set device's full path in device tree.
 *
 * @param node		The device's device node.
 * @param parent	The parent device node.
 * @return		True on success, false otherwise (insufficient
 *			resources etc.).
 */
static bool set_dev_path(node_t *node, node_t *parent)
{
	assert(node->name != NULL);
	
	size_t pathsize = (str_size(node->name) + 1);
	if (parent != NULL)
		pathsize += str_size(parent->pathname) + 1;
	
	node->pathname = (char *) malloc(pathsize);
	if (node->pathname == NULL) {
		printf(NAME ": failed to allocate device path.\n");
		return false;
	}
	
	if (parent != NULL) {
		str_cpy(node->pathname, pathsize, parent->pathname);
		str_append(node->pathname, pathsize, "/");
		str_append(node->pathname, pathsize, node->name);
	} else {
		str_cpy(node->pathname, pathsize, node->name);
	}
	
	return true;
}

/** Insert new device into device tree.
 *
 * The device tree's rwlock should be already held exclusively when calling this
 * function.
 *
 * @param tree		The device tree.
 * @param node		The newly added device node. 
 * @param dev_name	The name of the newly added device.
 * @param parent	The parent device node.
 *
 * @return		True on success, false otherwise (insufficient resources
 *			etc.).
 */
bool insert_dev_node(dev_tree_t *tree, node_t *node, char *dev_name,
    node_t *parent)
{
	assert(node != NULL);
	assert(tree != NULL);
	assert(dev_name != NULL);
	
	node->name = dev_name;
	if (!set_dev_path(node, parent)) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		return false;
	}
	
	/* Add the node to the handle-to-node map. */
	node->handle = ++tree->current_handle;
	unsigned long key = node->handle;
	hash_table_insert(&tree->devman_devices, &key, &node->devman_link);

	/* Add the node to the list of its parent's children. */
	node->parent = parent;
	if (parent != NULL)
		list_append(&node->sibling, &parent->children);
	
	return true;
}

/** Find device node with a specified path in the device tree.
 * 
 * @param path		The path of the device node in the device tree.
 * @param tree		The device tree.
 * @return		The device node if it is present in the tree, NULL
 *			otherwise.
 */
node_t *find_dev_node_by_path(dev_tree_t *tree, char *path)
{
	fibril_rwlock_read_lock(&tree->rwlock);
	
	node_t *dev = tree->root_node;
	/*
	 * Relative path to the device from its parent (but with '/' at the
	 * beginning)
	 */
	char *rel_path = path;
	char *next_path_elem = NULL;
	bool cont = (rel_path[0] == '/');
	
	while (cont && dev != NULL) {
		next_path_elem  = get_path_elem_end(rel_path + 1);
		if (next_path_elem[0] == '/') {
			cont = true;
			next_path_elem[0] = 0;
		} else {
			cont = false;
		}
		
		dev = find_node_child(dev, rel_path + 1);
		
		if (cont) {
			/* Restore the original path. */
			next_path_elem[0] = '/';
		}
		rel_path = next_path_elem;
	}
	
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return dev;
}

/** Find child device node with a specified name.
 *
 * Device tree rwlock should be held at least for reading.
 *
 * @param parent	The parent device node.
 * @param name		The name of the child device node.
 * @return		The child device node.
 */
node_t *find_node_child(node_t *parent, const char *name)
{
	node_t *dev;
	link_t *link;
	
	link = parent->children.next;
	
	while (link != &parent->children) {
		dev = list_get_instance(link, node_t, sibling);
		
		if (str_cmp(name, dev->name) == 0)
			return dev;
		
		link = link->next;
	}
	
	return NULL;
}

/* Device classes */

/** Create device class.
 *
 * @return	Device class.
 */
dev_class_t *create_dev_class(void)
{
	dev_class_t *cl;
	
	cl = (dev_class_t *) malloc(sizeof(dev_class_t));
	if (cl != NULL) {
		memset(cl, 0, sizeof(dev_class_t));
		list_initialize(&cl->devices);
		fibril_mutex_initialize(&cl->mutex);
	}
	
	return cl;
}

/** Create device class info.
 *
 * @return		Device class info.
 */
dev_class_info_t *create_dev_class_info(void)
{
	dev_class_info_t *info;
	
	info = (dev_class_info_t *) malloc(sizeof(dev_class_info_t));
	if (info != NULL)
		memset(info, 0, sizeof(dev_class_info_t));
	
	return info;
}

size_t get_new_class_dev_idx(dev_class_t *cl)
{
	size_t dev_idx;
	
	fibril_mutex_lock(&cl->mutex);
	dev_idx = ++cl->curr_dev_idx;
	fibril_mutex_unlock(&cl->mutex);
	
	return dev_idx;
}


/** Create unique device name within the class.
 *
 * @param cl		The class.
 * @param base_dev_name	Contains the base name for the device if it was
 *			specified by the driver when it registered the device by
 *			the class; NULL if driver specified no base name.
 * @return		The unique name for the device within the class.
 */
char *create_dev_name_for_class(dev_class_t *cl, const char *base_dev_name)
{
	char *dev_name;
	const char *base_name;
	
	if (base_dev_name != NULL)
		base_name = base_dev_name;
	else
		base_name = cl->base_dev_name;
	
	size_t idx = get_new_class_dev_idx(cl);
	asprintf(&dev_name, "%s%d", base_name, idx);
	
	return dev_name;
}

/** Add the device to the class.
 *
 * The device may be added to multiple classes and a class may contain multiple
 * devices. The class and the device are associated with each other by the
 * dev_class_info_t structure.
 *
 * @param dev		The device.
 * @param class		The class.
 * @param base_dev_name	The base name of the device within the class if
 *			specified by the driver, NULL otherwise.
 * @return		dev_class_info_t structure which associates the device
 *			with the class.
 */
dev_class_info_t *add_device_to_class(node_t *dev, dev_class_t *cl,
    const char *base_dev_name)
{
	dev_class_info_t *info = create_dev_class_info();
	
	if (info != NULL) {
		info->dev_class = cl;
		info->dev = dev;
		
		/* Add the device to the class. */
		fibril_mutex_lock(&cl->mutex);
		list_append(&info->link, &cl->devices);
		fibril_mutex_unlock(&cl->mutex);
		
		/* Add the class to the device. */
		list_append(&info->dev_classes, &dev->classes);
		
		/* Create unique name for the device within the class. */
		info->dev_name = create_dev_name_for_class(cl, base_dev_name);
	}
	
	return info;
}

dev_class_t *get_dev_class(class_list_t *class_list, char *class_name)
{
	dev_class_t *cl;
	
	fibril_rwlock_write_lock(&class_list->rwlock);
	cl = find_dev_class_no_lock(class_list, class_name);
	if (cl == NULL) {
		cl = create_dev_class();
		if (cl != NULL) {
			cl->name = class_name;
			cl->base_dev_name = "";
			add_dev_class_no_lock(class_list, cl);
		}
	}

	fibril_rwlock_write_unlock(&class_list->rwlock);
	return cl;
}

dev_class_t *find_dev_class_no_lock(class_list_t *class_list,
    const char *class_name)
{
	dev_class_t *cl;
	link_t *link = class_list->classes.next;
	
	while (link != &class_list->classes) {
		cl = list_get_instance(link, dev_class_t, link);
		if (str_cmp(cl->name, class_name) == 0)
			return cl;
	}
	
	return NULL;
}

void add_dev_class_no_lock(class_list_t *class_list, dev_class_t *cl)
{
	list_append(&cl->link, &class_list->classes);
}

void init_class_list(class_list_t *class_list)
{
	list_initialize(&class_list->classes);
	fibril_rwlock_initialize(&class_list->rwlock);
	hash_table_create(&class_list->devmap_devices, DEVICE_BUCKETS, 1,
	    &devmap_devices_ops);
}


/* Devmap devices */

node_t *find_devmap_tree_device(dev_tree_t *tree, devmap_handle_t devmap_handle)
{
	node_t *dev = NULL;
	link_t *link;
	unsigned long key = (unsigned long) devmap_handle;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	link = hash_table_find(&tree->devmap_devices, &key);
	if (link != NULL)
		dev = hash_table_get_instance(link, node_t, devmap_link);
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return dev;
}

node_t *find_devmap_class_device(class_list_t *classes,
    devmap_handle_t devmap_handle)
{
	node_t *dev = NULL;
	dev_class_info_t *cli;
	link_t *link;
	unsigned long key = (unsigned long)devmap_handle;
	
	fibril_rwlock_read_lock(&classes->rwlock);
	link = hash_table_find(&classes->devmap_devices, &key);
	if (link != NULL) {
		cli = hash_table_get_instance(link, dev_class_info_t,
		    devmap_link);
		dev = cli->dev;
	}
	fibril_rwlock_read_unlock(&classes->rwlock);
	
	return dev;
}

void class_add_devmap_device(class_list_t *class_list, dev_class_info_t *cli)
{
	unsigned long key = (unsigned long) cli->devmap_handle;
	
	fibril_rwlock_write_lock(&class_list->rwlock);
	hash_table_insert(&class_list->devmap_devices, &key, &cli->devmap_link);
	fibril_rwlock_write_unlock(&class_list->rwlock);
}

void tree_add_devmap_device(dev_tree_t *tree, node_t *node)
{
	unsigned long key = (unsigned long) node->devmap_handle;
	fibril_rwlock_write_lock(&tree->rwlock);
	hash_table_insert(&tree->devmap_devices, &key, &node->devmap_link);
	fibril_rwlock_write_unlock(&tree->rwlock);
}

/** @}
 */
