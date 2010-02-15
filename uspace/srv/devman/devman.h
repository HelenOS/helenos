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
#include <string.h>
#include <adt/list.h>
#include <ipc/ipc.h>

#include "util.h"

#define NAME "devman"

#define MATCH_EXT ".ma"

struct node;

typedef struct node node_t;

/** Ids of device models used for device-to-driver matching.
 */
typedef struct match_id {
	/** Pointers to next and previous ids.
	 */
	link_t link;
	/** Id of device model.
	 */
	const char *id;
	/** Relevancy of device-to-driver match.
	 * The higher is the product of scores specified for the device by the bus driver and by the leaf driver,
	 * the more suitable is the leaf driver for handling the device.
	 */
	unsigned int score;
} match_id_t;

/** List of ids for matching devices to drivers sorted
 * according to match scores in descending order.
 */
typedef struct match_id_list {
	link_t ids;
} match_id_list_t;

/** Representation of device driver.
 */
typedef struct driver {
	/** Pointers to previous and next drivers in a linked list */
	link_t drivers;
	/** Specifies whether the driver has been started.*/
	bool running;
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
} driver_t;

/** Representation of a node in the device tree.*/
struct node {
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
	/** Pointer to the previous and next device in the list of devices
	    owned by one driver */
	link_t driver_devices;
};

/** Represents device tree.
 */
typedef struct dev_tree {
	/** Root device node. */
	node_t *root_node;
} dev_tree_t;



// Match ids and scores

int get_match_score(driver_t *drv, node_t *dev);

bool parse_match_ids(const char *buf, match_id_list_t *ids);
bool read_match_ids(const char *conf_path, match_id_list_t *ids);
char * read_id(const char **buf) ;
void add_match_id(match_id_list_t *ids, match_id_t *id);

void clean_match_ids(match_id_list_t *ids);


static inline match_id_t * create_match_id()
{
	match_id_t *id = malloc(sizeof(match_id_t));
	memset(id, 0, sizeof(match_id_t));
	return id;
}

static inline void delete_match_id(match_id_t *id)
{
	if (id) {
		free_not_null(id->id);
		free(id);
	}
}

// Drivers

driver_t * create_driver();
bool get_driver_info(const char *base_path, const char *name, driver_t *drv);
int lookup_available_drivers(link_t *drivers_list, const char *dir_path);

driver_t * find_best_match_driver(link_t *drivers_list, node_t *node);
bool assign_driver(node_t *node, link_t *drivers_list);

void attach_driver(node_t *node, driver_t *drv);
bool add_device(driver_t *drv, node_t *node);
bool start_driver(driver_t *drv);


static inline void init_driver(driver_t *drv)
{
	printf(NAME ": init_driver\n");
	assert(drv != NULL);

	memset(drv, 0, sizeof(driver_t));
	list_initialize(&drv->match_ids.ids);
	list_initialize(&drv->devices);
}

static inline void clean_driver(driver_t *drv)
{
	printf(NAME ": clean_driver\n");
	assert(drv != NULL);

	free_not_null(drv->name);
	free_not_null(drv->binary_path); 

	clean_match_ids(&drv->match_ids);

	init_driver(drv);
}

static inline void delete_driver(driver_t *drv)
{
	printf(NAME ": delete_driver\n");
	assert(NULL != drv);
	
	clean_driver(drv);
	free(drv);
}

static inline void add_driver(link_t *drivers_list, driver_t *drv)
{
	list_prepend(&drv->drivers, drivers_list);
	printf(NAME": the '%s' driver was added to the list of available drivers.\n", drv->name);
}


// Device nodes
node_t * create_root_node();

static inline node_t * create_dev_node()
{
	node_t *res = malloc(sizeof(node_t));
	if (res != NULL) {
		memset(res, 0, sizeof(node_t));
	}
	return res;
}

static inline void init_dev_node(node_t *node, node_t *parent)
{
	assert(NULL != node);

	node->parent = parent;
	if (NULL != parent) {
		list_append(&node->sibling, &parent->children);
	}

	list_initialize(&node->children);

	list_initialize(&node->match_ids.ids);
}


// Device tree

bool init_device_tree(dev_tree_t *tree, link_t *drivers_list);


#endif
