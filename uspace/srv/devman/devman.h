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
#include <ipc/devman.h>
#include <ipc/devmap.h>
#include <fibril_synch.h>
#include <atomic.h>

#include "util.h"

#define NAME "devman"

#define MATCH_EXT ".ma"
#define DEVICE_BUCKETS 256

#define DEVMAP_CLASS_NAMESPACE "class"
#define DEVMAP_DEVICE_NAMESPACE "devices"
#define DEVMAP_SEPARATOR '\\'

struct dev_node;
typedef struct dev_node dev_node_t;

struct fun_node;
typedef struct fun_node fun_node_t;

typedef enum {
	/** Driver has not been started. */
	DRIVER_NOT_STARTED = 0,
	
	/**
	 * Driver has been started, but has not registered as running and ready
	 * to receive requests.
	 */
	DRIVER_STARTING,
	
	/** Driver is running and prepared to serve incomming requests. */
	DRIVER_RUNNING
} driver_state_t;

/** Representation of device driver. */
typedef struct driver {
	/** Pointers to previous and next drivers in a linked list. */
	link_t drivers;
	
	/**
	 * Specifies whether the driver has been started and wheter is running
	 * and prepared to receive requests.
	 */
	int state;
	
	/** Phone asociated with this driver. */
	sysarg_t phone;
	/** Name of the device driver. */
	char *name;
	/** Path to the driver's binary. */
	const char *binary_path;
	/** List of device ids for device-to-driver matching. */
	match_id_list_t match_ids;
	/** Pointer to the linked list of devices controlled by this driver. */
	link_t devices;
	
	/**
	 * Fibril mutex for this driver - driver state, list of devices, phone.
	 */
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

/** Device node in the device tree. */
struct dev_node {
	/** The global unique identifier of the device. */
	devman_handle_t handle;
	
	/** (Parent) function the device is attached to. */
	fun_node_t *pfun;
	
	/** List of device functions. */
	link_t functions;
	/** Driver of this device. */
	driver_t *drv;
	/** The state of the device. */
	device_state_t state;
	/** Link to list of devices owned by driver (driver_t.devices) */
	link_t driver_devices;
	
	/**
	 * Used by the hash table of devices indexed by devman device handles.
	 */
	link_t devman_dev;
	
	/**
	 * Whether this device was already passed to the driver.
	 */
	bool passed_to_driver;
};

/** Function node in the device tree. */
struct fun_node {
	/** The global unique identifier of the function */
	devman_handle_t handle;
	/** Name of the function, assigned by the device driver */
	char *name;
	
	/** Full path and name of the device in device hierarchy */
	char *pathname;
	
	/** Device which this function belongs to */
	dev_node_t *dev;
	
	/** Link to list of functions in the device (device_t.functions) */
	link_t dev_functions;
	
	/** Child device node (if any attached). */
	dev_node_t *child;
	/** List of device ids for device-to-driver matching. */
	match_id_list_t match_ids;
	
	/** The list of device classes to which this device function belongs. */
	link_t classes;
	/** Devmap handle if the device function is registered by devmap. */
	devmap_handle_t devmap_handle;
	
	/**
	 * Used by the hash table of functions indexed by devman device handles.
	 */
	link_t devman_fun;
	
	/**
	 * Used by the hash table of functions indexed by devmap device handles.
	 */
	link_t devmap_fun;
};


/** Represents device tree. */
typedef struct dev_tree {
	/** Root device node. */
	fun_node_t *root_node;
	
	/**
	 * The next available handle - handles are assigned in a sequential
	 * manner.
	 */
	devman_handle_t current_handle;
	
	/** Synchronize access to the device tree. */
	fibril_rwlock_t rwlock;
	
	/** Hash table of all devices indexed by devman handles. */
	hash_table_t devman_devices;
	
	/** Hash table of all devices indexed by devman handles. */
	hash_table_t devman_functions;
	
	/**
	 * Hash table of devices registered by devmapper, indexed by devmap
	 * handles.
	 */
	hash_table_t devmap_functions;
} dev_tree_t;

typedef struct dev_class {
	/** The name of the class. */
	const char *name;
	
	/**
	 * Pointer to the previous and next class in the list of registered
	 * classes.
	 */
	link_t link;
	
	/**
	 * List of dev_class_info structures - one for each device registered by
	 * this class.
	 */
	link_t devices;
	
	/**
	 * Default base name for the device within the class, might be overrided
	 * by the driver.
	 */
	const char *base_dev_name;
	
	/** Unique numerical identifier of the newly added device. */
	size_t curr_dev_idx;
	/** Synchronize access to the list of devices in this class. */
	fibril_mutex_t mutex;
} dev_class_t;

/**
 * Provides n-to-m mapping between function nodes and classes - each function
 * can register in an arbitrary number of classes and each class can contain
 * an arbitrary number of device functions.
 */
typedef struct dev_class_info {
	/** The class. */
	dev_class_t *dev_class;
	/** The device. */
	fun_node_t *fun;
	
	/**
	 * Pointer to the previous and next class info in the list of devices
	 * registered by the class.
	 */
	link_t link;
	
	/**
	 * Pointer to the previous and next class info in the list of classes
	 * by which the device is registered.
	 */
	link_t dev_classes;
	
	/** The name of the device function within the class. */
	char *dev_name;
	/** The handle of the device by device mapper in the class namespace. */
	devmap_handle_t devmap_handle;
	
	/**
	 * Link in the hash table of devices registered by the devmapper using
	 * their class names.
	 */
	link_t devmap_link;
} dev_class_info_t;

/** The list of device classes. */
typedef struct class_list {
	/** List of classes. */
	link_t classes;
	
	/**
	 * Hash table of devices registered by devmapper using their class name,
	 * indexed by devmap handles.
	 */
	hash_table_t devmap_functions;
	
	/** Fibril mutex for list of classes. */
	fibril_rwlock_t rwlock;
} class_list_t;

/* Match ids and scores */

extern int get_match_score(driver_t *, dev_node_t *);

extern bool parse_match_ids(char *, match_id_list_t *);
extern bool read_match_ids(const char *, match_id_list_t *);
extern char *read_match_id(char **);
extern char *read_id(const char **);

/* Drivers */

extern void init_driver_list(driver_list_t *);
extern driver_t *create_driver(void);
extern bool get_driver_info(const char *, const char *, driver_t *);
extern int lookup_available_drivers(driver_list_t *, const char *);

extern driver_t *find_best_match_driver(driver_list_t *, dev_node_t *);
extern bool assign_driver(dev_node_t *, driver_list_t *, dev_tree_t *);

extern void add_driver(driver_list_t *, driver_t *);
extern void attach_driver(dev_node_t *, driver_t *);
extern void add_device(int, driver_t *, dev_node_t *, dev_tree_t *);
extern bool start_driver(driver_t *);

extern driver_t *find_driver(driver_list_t *, const char *);
extern void set_driver_phone(driver_t *, sysarg_t);
extern void initialize_running_driver(driver_t *, dev_tree_t *);

extern void init_driver(driver_t *);
extern void clean_driver(driver_t *);
extern void delete_driver(driver_t *);

/* Device nodes */

extern dev_node_t *create_dev_node(void);
extern void delete_dev_node(dev_node_t *node);
extern dev_node_t *find_dev_node_no_lock(dev_tree_t *tree,
    devman_handle_t handle);
extern dev_node_t *find_dev_node(dev_tree_t *tree, devman_handle_t handle);
extern dev_node_t *find_dev_function(dev_node_t *, const char *);

extern fun_node_t *create_fun_node(void);
extern void delete_fun_node(fun_node_t *);
extern fun_node_t *find_fun_node_no_lock(dev_tree_t *tree,
    devman_handle_t handle);
extern fun_node_t *find_fun_node(dev_tree_t *tree, devman_handle_t handle);
extern fun_node_t *find_fun_node_by_path(dev_tree_t *, char *);

/* Device tree */

extern bool init_device_tree(dev_tree_t *, driver_list_t *);
extern bool create_root_nodes(dev_tree_t *);
extern bool insert_dev_node(dev_tree_t *, dev_node_t *, fun_node_t *);
extern bool insert_fun_node(dev_tree_t *, fun_node_t *, char *, dev_node_t *);

/* Device classes */

extern dev_class_t *create_dev_class(void);
extern dev_class_info_t *create_dev_class_info(void);
extern size_t get_new_class_dev_idx(dev_class_t *);
extern char *create_dev_name_for_class(dev_class_t *, const char *);
extern dev_class_info_t *add_function_to_class(fun_node_t *, dev_class_t *,
    const char *);

extern void init_class_list(class_list_t *);

extern dev_class_t *get_dev_class(class_list_t *, char *);
extern dev_class_t *find_dev_class_no_lock(class_list_t *, const char *);
extern void add_dev_class_no_lock(class_list_t *, dev_class_t *);

/* Devmap devices */

extern fun_node_t *find_devmap_tree_function(dev_tree_t *, devmap_handle_t);
extern fun_node_t *find_devmap_class_function(class_list_t *, devmap_handle_t);

extern void class_add_devmap_function(class_list_t *, dev_class_info_t *);
extern void tree_add_devmap_function(dev_tree_t *, fun_node_t *);

#endif

/** @}
 */
