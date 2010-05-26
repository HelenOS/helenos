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

#ifndef DEVMAN_H_
#define DEVMAN_H_

#include <assert.h>
#include <bool.h>
#include <dirent.h>
#include <str.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <ipc/ipc.h>
#include <ipc/devman.h>
#include <ipc/devmap.h>
#include <fibril_synch.h>
#include <atomic.h>

#include "util.h"

#define NAME "devman"

#define MATCH_EXT ".ma"
#define DEVICE_BUCKETS 256

struct node;
typedef struct node node_t;

typedef enum {
	/** driver has not been started */
	DRIVER_NOT_STARTED = 0,
	/** driver has been started, but has not registered as running and ready to receive requests */
	DRIVER_STARTING,
	/** driver is running and prepared to serve incomming requests */
	DRIVER_RUNNING
} driver_state_t;

/** Representation of device driver.
 */
typedef struct driver {
	/** Pointers to previous and next drivers in a linked list */
	link_t drivers;
	/** Specifies whether the driver has been started and wheter is running and prepared to receive requests.*/
	int state;
	/** Phone asociated with this driver */
	ipcarg_t phone;
	/** Name of the device driver */
	char *name;
	/** Path to the driver's binary */
	const char *binary_path;
	/** List of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
	/** Pointer to the linked list of devices controlled by this driver */
	link_t devices;
	/** Fibril mutex for this driver - driver state, list of devices, phone.*/
	fibril_mutex_t driver_mutex;
} driver_t;

/** The list of drivers. */
typedef struct driver_list {
	/** List of drivers */
	link_t drivers;
	/** Fibril mutex for list of drivers. */
	fibril_mutex_t drivers_mutex;	
} driver_list_t;

/** The state of the device. */
typedef enum {
	DEVICE_NOT_INITIALIZED = 0,
	DEVICE_USABLE,
	DEVICE_NOT_PRESENT,
	DEVICE_INVALID
} device_state_t;

/** Representation of a node in the device tree.*/
struct node {
	/** The global unique identifier of the device.*/
	device_handle_t handle;
	/** The name of the device specified by its parent. */
	char *name;
	/** Full path and name of the device in device hierarchi (i. e. in full path in device tree).*/
	char *pathname;	
	/** The node of the parent device. */
	node_t *parent;
	/** Pointers to previous and next child devices in the linked list of parent device's node.*/
	link_t sibling;
	/** List of child device nodes. */
	link_t children;
	/** List of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
	/** Driver of this device.*/
	driver_t *drv;
	/** The state of the device. */
	device_state_t state;
	/** Pointer to the previous and next device in the list of devices
	    owned by one driver */
	link_t driver_devices;
	/** The list of device classes to which this device belongs.*/
	link_t classes;
	/** Devmap handle if the device is registered by devmapper. */
	dev_handle_t devmap_handle;
	/** Used by the hash table of devices indexed by devman device handles.*/
	link_t devman_link;
	/** Used by the hash table of devices indexed by devmap device handles.*/
	link_t devmap_link;
};


/** Represents device tree.
 */
typedef struct dev_tree {
	/** Root device node. */
	node_t *root_node;
	/** The next available handle - handles are assigned in a sequential manner.*/
	device_handle_t current_handle;
	/** Synchronize access to the device tree.*/
	fibril_rwlock_t rwlock;
	/** Hash table of all devices indexed by devman handles.*/
	hash_table_t devman_devices;
	/** Hash table of devices registered by devmapper, indexed by devmap handles.*/
	hash_table_t devmap_devices;
	
} dev_tree_t;

typedef struct dev_class {	
	/** The name of the class.*/
	const char *name;
	/** Pointer to the previous and next class in the list of registered classes.*/
	link_t link;	
	/** List of dev_class_info structures - one for each device registered by this class.*/
	link_t devices;	
	/** Default base name for the device within the class, might be overrided by the driver.*/
	const char *base_dev_name;
	/** Unique numerical identifier appened to the base name of the newly added device.*/
	size_t curr_dev_idx;
	/** Synchronize access to the list of devices in this class. */
	fibril_mutex_t mutex;
} dev_class_t;

/** Provides n-to-m mapping between device nodes and classes 
 * - each device may be register to the arbitrary number of classes 
 * and each class may contain the arbitrary number of devices. */
typedef struct dev_class_info {
	/** The class.*/
	dev_class_t *dev_class;
	/** The device.*/
	node_t *dev;
	/** Pointer to the previous and next class info in the list of devices registered by the class.*/
	link_t link;
	/** Pointer to the previous and next class info in the list of classes by which the device is registered.*/
	link_t dev_classes;
	/** The name of the device within the class.*/
	char *dev_name;	
	/** The handle of the device by device mapper in the class namespace.*/
	dev_handle_t devmap_handle;
} dev_class_info_t;

// Match ids and scores

int get_match_score(driver_t *drv, node_t *dev);

bool parse_match_ids(char *buf, match_id_list_t *ids);
bool read_match_ids(const char *conf_path, match_id_list_t *ids);
char * read_match_id(char **buf);
char * read_id(const char **buf);

// Drivers

/** 
 * Initialize the list of device driver's.
 * 
 * @param drv_list the list of device driver's.
 * 
 */
static inline void init_driver_list(driver_list_t *drv_list) 
{
	assert(NULL != drv_list);
	
	list_initialize(&drv_list->drivers);
	fibril_mutex_initialize(&drv_list->drivers_mutex);	
}

driver_t * create_driver(void);
bool get_driver_info(const char *base_path, const char *name, driver_t *drv);
int lookup_available_drivers(driver_list_t *drivers_list, const char *dir_path);

driver_t * find_best_match_driver(driver_list_t *drivers_list, node_t *node);
bool assign_driver(node_t *node, driver_list_t *drivers_list);

void add_driver(driver_list_t *drivers_list, driver_t *drv);
void attach_driver(node_t *node, driver_t *drv);
void add_device(int phone, driver_t *drv, node_t *node);
bool start_driver(driver_t *drv);

driver_t * find_driver(driver_list_t *drv_list, const char *drv_name);
void set_driver_phone(driver_t *driver, ipcarg_t phone);
void initialize_running_driver(driver_t *driver);

/** 
 * Initialize device driver structure.
 * 
 * @param drv the device driver structure.
 * 
 */
static inline void init_driver(driver_t *drv)
{
	assert(drv != NULL);

	memset(drv, 0, sizeof(driver_t));
	list_initialize(&drv->match_ids.ids);
	list_initialize(&drv->devices);
	fibril_mutex_initialize(&drv->driver_mutex);	
}

/**
 * Device driver structure clean-up.
 * 
 * @param drv the device driver structure. 
 */
static inline void clean_driver(driver_t *drv)
{
	assert(drv != NULL);

	free_not_null(drv->name);
	free_not_null(drv->binary_path); 

	clean_match_ids(&drv->match_ids);

	init_driver(drv);
}

/**
 * Delete device driver structure.
 * 
 *  @param drv the device driver structure.* 
 */
static inline void delete_driver(driver_t *drv)
{
	assert(NULL != drv);
	
	clean_driver(drv);
	free(drv);
}

// Device nodes
/**
 * Create a new device node.
 * 
 * @return a device node structure.
 * 
 */
static inline node_t * create_dev_node()
{
	node_t *res = malloc(sizeof(node_t));
	if (res != NULL) {
		memset(res, 0, sizeof(node_t));
	}
	
	list_initialize(&res->children);
	list_initialize(&res->match_ids.ids);
	
	return res;
}

/**
 * Delete a device node.
 * 
 * @param node a device node structure.
 * 
 */
static inline void delete_dev_node(node_t *node)
{
	assert(list_empty(&node->children) && NULL == node->parent && NULL == node->drv);
	
	clean_match_ids(&node->match_ids);
	free_not_null(node->name);
	free_not_null(node->pathname);
	free(node);	
}

/**
 * Find the device node structure of the device witch has the specified handle.
 * 
 * Device tree's rwlock should be held at least for reading.
 * 
 * @param tree the device tree where we look for the device node.
 * @param handle the handle of the device.
 * @return the device node. 
 */
static inline node_t * find_dev_node_no_lock(dev_tree_t *tree, device_handle_t handle)
{
	unsigned long key = handle;
	link_t *link = hash_table_find(&tree->devman_devices, &key);
	return hash_table_get_instance(link, node_t, devman_link);
}

/**
 * Find the device node structure of the device witch has the specified handle.
 * 
 * @param tree the device tree where we look for the device node.
 * @param handle the handle of the device.
 * @return the device node. 
 */
static inline node_t * find_dev_node(dev_tree_t *tree, device_handle_t handle)
{
	node_t *node = NULL;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	
	node = find_dev_node_no_lock(tree, handle);
	
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return node;
}

node_t * find_dev_node_by_path(dev_tree_t *tree, char *path);
node_t *find_node_child(node_t *parent, const char *name);

// Device tree

bool init_device_tree(dev_tree_t *tree, driver_list_t *drivers_list);
bool create_root_node(dev_tree_t *tree);
bool insert_dev_node(dev_tree_t *tree, node_t *node, char *dev_name, node_t *parent);

// Device classes

/** Create device class. 
 *   
 * @return device class.
 */
static inline dev_class_t * create_dev_class()
{
	dev_class_t *cl = (dev_class_t *)malloc(sizeof(dev_class_t));
	if (NULL != cl) {
		memset(cl, 0, sizeof(dev_class_t));
		fibril_mutex_initialize(&cl->mutex);
	}
	return cl;	
}

/** Create device class info. 
 * 
 * @return device class info.
 */
static inline dev_class_info_t * create_dev_class_info()
{
	dev_class_info_t *info = (dev_class_info_t *)malloc(sizeof(dev_class_info_t));
	if (NULL != info) {
		memset(info, 0, sizeof(dev_class_info_t));
	}
	return info;	
}

static inline size_t get_new_class_dev_idx(dev_class_t *cl)
{
	size_t dev_idx;
	fibril_mutex_lock(&cl->mutex);
	dev_idx = ++cl->curr_dev_idx;
	fibril_mutex_unlock(&cl->mutex);
	return dev_idx;
}

char * create_dev_name_for_class(dev_class_t *cl, const char *base_dev_name);
dev_class_info_t * add_device_to_class(node_t *dev, dev_class_t *cl, const char *base_dev_name);

#endif

/** @}
 */