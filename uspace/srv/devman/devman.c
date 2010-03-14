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

#include "devman.h"
#include "util.h"

/** Allocate and initialize a new driver structure.
 * 
 * @return driver structure.
 */
driver_t * create_driver() 
{	
	driver_t *res = malloc(sizeof(driver_t));
	if(res != NULL) {
		init_driver(res);
	}
	return res;
}

/** Add a driver to the list of drivers.
 * 
 * @param drivers_list the list of drivers.
 * @param drv the driver's structure.
 */
void add_driver(driver_list_t *drivers_list, driver_t *drv)
{
	fibril_mutex_lock(&drivers_list->drivers_mutex);
	list_prepend(&drv->drivers, &drivers_list->drivers);
	fibril_mutex_unlock(&drivers_list->drivers_mutex);
	
	printf(NAME": the '%s' driver was added to the list of available drivers.\n", drv->name);
}

/** Read match id at the specified position of a string and set 
 * the position in the string to the first character following the id.
 * 
 * @param buf the position in the input string.
 * 
 * @return the match id. 
 */
char * read_match_id(const char **buf) 
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
 * @param buf the string from which the match ids are read.
 * @param ids the list of match ids into which the match ids and scores are added.
 * 
 * @return true if at least one match id and associated match score was successfully read, false otherwise.
 */
bool parse_match_ids(const char *buf, match_id_list_t *ids)
{
	int score = 0;
	char *id = NULL;
	int ids_read = 0;
	
	while (true) {
		// skip spaces
		if (!skip_spaces(&buf)) {
			break;
		}
		// read score
		score = strtoul(buf, &buf, 10);
		
		// skip spaces
		if (!skip_spaces(&buf)) {
			break;
		}
		
		// read id
		if (NULL == (id = read_match_id(&buf))) {
			break;			
		}
		
		// create new match_id structure
		match_id_t *mid = create_match_id();
		mid->id = id;
		mid->score = score;
		
		/// add it to the list
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
 * @param buf the path to the file from which the match ids are read.
 * @param ids the list of match ids into which the match ids and scores are added.
 * 
 * @return true if at least one match id and associated match score was successfully read, false otherwise.
 */
bool read_match_ids(const char *conf_path, match_id_list_t *ids) 
{	
	printf(NAME ": read_match_ids conf_path = %s.\n", conf_path);
	
	bool suc = false;	
	char *buf = NULL;
	bool opened = false;
	int fd;		
	off_t len = 0;
	
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
		printf(NAME ": memory allocation failed when parsing file '%s'.\n", conf_path);
		goto cleanup;
	}	
	
	if (0 >= read(fd, buf, len)) {
		printf(NAME ": unable to read file '%s'.\n", conf_path);
		goto cleanup;
	}
	buf[len] = 0;
	
	suc = parse_match_ids(buf, ids);
	
cleanup:
	
	free(buf);
	
	if(opened) {
		close(fd);	
	}
	
	return suc;
}

/**
 * Get information about a driver.
 * 
 * Each driver has its own directory in the base directory. 
 * The name of the driver's directory is the same as the name of the driver.
 * The driver's directory contains driver's binary (named as the driver without extension)
 * and the configuration file with match ids for device-to-driver matching 
 * (named as the driver with a special extension).
 * 
 * This function searches for the driver's directory and containing configuration files.
 * If all the files needed are found, they are parsed and 
 * the information about the driver is stored to the driver's structure.
 * 
 * @param base_path the base directory, in which we look for driver's subdirectory.
 * @param name the name of the driver.
 * @param drv the driver structure to fill information in.
 * 
 * @return true on success, false otherwise.
 */
bool get_driver_info(const char *base_path, const char *name, driver_t *drv)
{
	printf(NAME ": get_driver_info base_path = %s, name = %s.\n", base_path, name);
	
	assert(base_path != NULL && name != NULL && drv != NULL);
	
	bool suc = false;
	char *match_path = NULL;	
	size_t name_size = 0;
	
	// read the list of match ids from the driver's configuration file
	if (NULL == (match_path = get_abs_path(base_path, name, MATCH_EXT))) {
		goto cleanup;
	}	
	
	printf(NAME ": get_driver_info - path to match id list = %s.\n", match_path);
	
	if (!read_match_ids(match_path, &drv->match_ids)) {
		goto cleanup;
	}	
	
	// allocate and fill driver's name
	name_size = str_size(name)+1;
	drv->name = malloc(name_size);
	if (!drv->name) {
		goto cleanup;
	}	
	str_cpy(drv->name, name_size, name);
	
	// initialize path with driver's binary
	if (NULL == (drv->binary_path = get_abs_path(base_path, name, ""))) {
		goto cleanup;
	}
	
	// check whether the driver's binary exists
	struct stat s;
	if (stat(drv->binary_path, &s) == ENOENT) {
		printf(NAME ": driver not found at path %s.", drv->binary_path);
		goto cleanup;
	}
	
	suc = true;
	
cleanup:
	
	if (!suc) {
		free(drv->binary_path);
		free(drv->name);
		// set the driver structure to the default state
		init_driver(drv); 
	}
	
	free(match_path);
	
	return suc;
}

/** Lookup drivers in the directory.
 * 
 * @param drivers_list the list of available drivers.
 * @param dir_path the path to the directory where we search for drivers. 
 * 
 * @return number of drivers which were found.
 */ 
int lookup_available_drivers(driver_list_t *drivers_list, const char *dir_path)
{
	printf(NAME ": lookup_available_drivers \n");
	
	int drv_cnt = 0;
	DIR *dir = NULL;
	struct dirent *diren;

	dir = opendir(dir_path);
	printf(NAME ": lookup_available_drivers has opened directory %s for driver search.\n", dir_path);
	
	if (dir != NULL) {
		driver_t *drv = create_driver();
		printf(NAME ": lookup_available_drivers has created driver structure.\n");
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
 * @param tree the device tree.
 * @return true on success, false otherwise.
 */
bool create_root_node(dev_tree_t *tree)
{
	printf(NAME ": create_root_node\n");
	node_t *node = create_dev_node();
	if (node) {
		insert_dev_node(tree, node, NULL);
		match_id_t *id = create_match_id();
		id->id = "root";
		id->score = 100;
		add_match_id(&node->match_ids, id);
		tree->root_node = node;
	}
	return node != NULL;	
}

/** Lookup the best matching driver for the specified device in the list of drivers.
 * 
 * A match between a device and a driver is found 
 * if one of the driver's match ids match one of the device's match ids.
 * The score of the match is the product of the driver's and device's score associated with the matching id.
 * The best matching driver for a device is the driver
 * with the highest score of the match between the device and the driver.
 * 
 * @param drivers_list the list of drivers, where we look for the driver suitable for handling the device.
 * @param node the device node structure of the device.
 *
 * @return the best matching driver or NULL if no matching driver is found.
 */
driver_t * find_best_match_driver(driver_list_t *drivers_list, node_t *node)
{
	printf(NAME ": find_best_match_driver\n");
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

/**
 * Assign a driver to a device.
 * 
 * @param node the device's node in the device tree.
 * @param drv the driver.
 */
void attach_driver(node_t *node, driver_t *drv) 
{
	fibril_mutex_lock(&drv->driver_mutex);
	
	node->drv = drv;
	list_append(&node->driver_devices, &drv->devices);
	
	fibril_mutex_unlock(&drv->driver_mutex);
}

/** Start a driver.
 * 
 * The driver's mutex is assumed to be locked.
 * 
 * @param drv the driver's structure.
 * @return true if the driver's task is successfully spawned, false otherwise.
 */
bool start_driver(driver_t *drv)
{
	printf(NAME ": start_driver\n");
	
	char *argv[2];
	
	printf(NAME ": spawning driver %s\n", drv->name);
	
	argv[0] = drv->name;
	argv[1] = NULL;
	
	if (!task_spawn(drv->binary_path, argv)) {
		printf(NAME ": error spawning %s\n", drv->name);
		return false;
	}
	
	drv->state = DRIVER_STARTING;
	return true;
}

driver_t * find_driver(driver_list_t *drv_list, const char *drv_name) 
{	
	driver_t *res = NULL;
	
	fibril_mutex_lock(&drv_list->drivers_mutex);	
	
	driver_t *drv = NULL;
	link_t *link = drv_list->drivers.next; 	
	while (link !=  &drv_list->drivers) {
		drv = list_get_instance(link, driver_t, drivers);
		if (0 == str_cmp(drv->name, drv_name)) {
			res = drv;
			break;
		}		
		link = link->next;
	}	
	
	fibril_mutex_unlock(&drv_list->drivers_mutex);
	
	return res;
}

void set_driver_phone(driver_t *driver, ipcarg_t phone)
{		
	fibril_mutex_lock(&driver->driver_mutex);	
	assert(DRIVER_STARTING == driver->state);
	driver->phone = phone;	
	fibril_mutex_unlock(&driver->driver_mutex);	
}

/**
 * Notify driver about the devices to which it was assigned.
 * 
 * The driver's mutex must be locked.
 * 
 * @param driver the driver to which the devices are passed.
 */
static void pass_devices_to_driver(driver_t *driver)
{	
	node_t *dev;
	link_t *link;
	
	int phone = ipc_connect_me_to(driver->phone, DRIVER_DEVMAN, 0, 0);
	
	if (0 < phone) {
		
		link = driver->devices.next;
		while (link != &driver->devices) {
			dev = list_get_instance(link, node_t, driver_devices);
			add_device(phone, driver, dev);
			link = link->next;
		}
		
		ipc_hangup(phone);
	}
}

/** Finish the initialization of a driver after it has succesfully started and registered itself by the device manager.
 * 
 * Pass devices formerly matched to the driver to the driver and remember the driver is running and fully functional now.
 * 
 * @param driver the driver which registered itself as running by the device manager.
 */
void initialize_running_driver(driver_t *driver) 
{	
	fibril_mutex_lock(&driver->driver_mutex);
	
	// pass devices which have been already assigned to the driver to the driver
	pass_devices_to_driver(driver);	
	
	// change driver's state to running
	driver->state = DRIVER_RUNNING;	
	
	fibril_mutex_unlock(&driver->driver_mutex);
}

/** Pass a device to running driver.
 * 
 * @param drv the driver's structure.
 * @param node the device's node in the device tree.
 */
void add_device(int phone, driver_t *drv, node_t *node)
{
	printf(NAME ": add_device\n");

	ipcarg_t ret;
	ipcarg_t rc = async_req_1_1(phone, DRIVER_ADD_DEVICE, node->handle, &ret);
	if (rc != EOK) {
		// TODO handle error
		return false;
	}
	
	// TODO inspect return value (ret) to find out whether the device was successfully probed and added
	
	return true;
}

/** 
 * Find suitable driver for a device and assign the driver to it.
 * 
 * @param node the device node of the device in the device tree.
 * @param drivers_list the list of available drivers.
 * 
 * @return true if the suitable driver is found and successfully assigned to the device, false otherwise. 
 */
bool assign_driver(node_t *node, driver_list_t *drivers_list) 
{
	printf(NAME ": assign_driver\n");
	
	// find the driver which is the most suitable for handling this device
	driver_t *drv = find_best_match_driver(drivers_list, node);
	if (NULL == drv) {
		printf(NAME ": no driver found for device.\n"); 
		return false;		
	}
	
	// attach the driver to the device
	attach_driver(node, drv);
	
	if (DRIVER_NOT_STARTED == drv->state) {
		// start driver
		start_driver(drv);
	} 
	
	if (DRIVER_RUNNING == drv->state) {
		// notify driver about new device
		int phone = ipc_connect_me_to(drv->phone, DRIVER_DEVMAN, 0, 0);
		if (phone > 0) {
			add_device(phone, drv, node);		
			ipc_hangup(phone);
		}
	}
	
	return true;
}

/**
 * Initialize the device tree.
 * 
 * Create root device node of the tree and assign driver to it.
 * 
 * @param tree the device tree.
 * @param the list of available drivers.
 * @return true on success, false otherwise.
 */
bool init_device_tree(dev_tree_t *tree, driver_list_t *drivers_list)
{
	printf(NAME ": init_device_tree.\n");
	
	atomic_set(&tree->current_handle, 0);
	
	// create root node and add it to the device tree
	if (!create_root_node(tree)) {
		return false;
	}

	// find suitable driver and start it
	return assign_driver(tree->root_node, drivers_list);
}

/** @}
 */