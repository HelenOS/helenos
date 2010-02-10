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

/**
 * @defgroup devman Device manager.
 * @brief HelenOS device manager.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
//#include <ipc/devman.h>

#define NAME          "devman"

#define DRIVER_DEFAULT_STORE  "/srv/drivers"
#define MATCH_EXT ".ma"

#define MAX_ID_LEN 256


struct driver;
struct match_id;
struct match_id_list;
struct node;
struct dev_tree;


typedef struct driver driver_t;
typedef struct match_id match_id_t;
typedef struct match_id_list match_id_list_t;
typedef struct node node_t;
typedef struct dev_tree dev_tree_t;


static driver_t * create_driver();
static inline void init_driver(driver_t *drv);
static inline void clean_driver(driver_t *drv);
static inline void delete_driver(driver_t *drv);
static inline void add_driver(driver_t *drv);
static char * get_abs_path(const char *base_path, const char *name, const char *ext);
static bool parse_match_ids(const char *buf, match_id_list_t *ids);
static bool read_match_ids(const char *conf_path, match_id_list_t *ids);
static void clean_match_ids(match_id_list_t *ids);
static inline match_id_t * create_match_id();
static inline void delete_match_id(match_id_t *id);
static void add_match_id(match_id_list_t *ids, match_id_t *id);
static bool get_driver_info(const char *base_path, const char *name, driver_t *drv);
static int lookup_available_drivers(const char *dir_path);
static inline node_t * create_dev_node();
static node_t * create_root_node();
static void init_device_tree(dev_tree_t *tree);
static bool devman_init();



LIST_INITIALIZE(drivers_list);



/** Represents device tree.
 */
struct dev_tree {
	node_t *root_node;
};

static dev_tree_t device_tree;

/** Ids of device models used for device-to-driver matching.
 */
struct match_id {
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
};

/** List of ids for matching devices to drivers sorted
 * according to match scores in descending order.
 */
struct match_id_list {
	link_t ids;
};

/** Representation of device driver.
 */
struct driver {
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
};

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
};

static driver_t * create_driver() 
{
	driver_t *res = malloc(sizeof(driver_t));
	if(res != NULL) {
		clean_driver(res);
	}
	return res;
}

static inline void init_driver(driver_t *drv) 
{
	assert(drv != NULL);	
	
	memset(drv, 0, sizeof(driver_t));	
	list_initialize(&drv->match_ids.ids);
}

static inline void clean_driver(driver_t *drv) 
{
	assert(drv != NULL);
	
	free(drv->name);
	free(drv->binary_path);
	
	clean_match_ids(&drv->match_ids);
	
	init_driver(drv);	
}

static void clean_match_ids(match_id_list_t *ids)
{
	link_t *link = NULL;
	match_id_t *id;
	
	while(!list_empty(&ids->ids)) {
		link = ids->ids.next;
		list_remove(link);		
		id = list_get_instance(link, match_id_t, link);
		delete_match_id(id);		
	}	
}

static inline match_id_t * create_match_id() 
{
	match_id_t *id = malloc(sizeof(match_id_t));
	memset(id, 0, sizeof(match_id_t));
	return id;	
}

static inline void delete_match_id(match_id_t *id) 
{
	free(id->id);
	free(id);
}

static void add_match_id(match_id_list_t *ids, match_id_t *id) 
{
	match_id_t *mid = NULL;
	link_t *link = ids->ids.next;	
	
	while (link != &ids->ids) {
		mid = list_get_instance(link, match_id_t,link);
		if (mid->score < id->score) {
			break;
		}	
		link = link->next;
	}
	
	list_insert_before(&id->link, link);	
}

static inline void delete_driver(driver_t *drv) 
{
	clean_driver(drv);
	free(drv);
}

static inline void add_driver(driver_t *drv)
{
	list_prepend(&drv->drivers, &drivers_list);
	printf(NAME": the '%s' driver was added to the list of available drivers.\n", drv->name);	
}

static char * get_abs_path(const char *base_path, const char *name, const char *ext) 
{
	char *res;
	int base_len = str_size(base_path);
	int size = base_len + str_size(name) + str_size(ext) + 3;	
	
	res = malloc(size);
	
	if (res) {
		str_cpy(res, size, base_path);
		if(base_path[base_len - 1] != '/') { 
			str_append(res, size, "/");			
		}
		str_append(res, size, name);
		if(ext[0] != '.') {
			str_append(res, size, ".");
		}
		str_append(res, size, ext);		
	}
	
	return res;
}

static inline bool skip_spaces(const char **buf) 
{
	while (isspace(**buf)) {
		(*buf)++;		
	}
	return *buf != 0;	
}

static inline size_t get_id_len(const char *str) 
{
	size_t len = 0;
	while(*str != 0 && !isspace(*str)) {
		len++;
		str++;
	}
	return len;
}

static char * read_id(const char **buf) 
{
	char *res = NULL;
	size_t len = get_id_len(*buf);
	if (len > 0) {
		res = malloc(len + 1);
		if (res != NULL) {
			str_ncpy(res, len + 1, *buf, len);	
			*buf += len;
		}
	}
	return res;
}

static bool parse_match_ids(const char *buf, match_id_list_t *ids)
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
		if (NULL == (id = read_id(&buf))) {
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

static bool read_match_ids(const char *conf_path, match_id_list_t *ids) 
{	
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


static bool get_driver_info(const char *base_path, const char *name, driver_t *drv)
{
	assert(base_path != NULL && name != NULL && drv != NULL);
	
	bool suc = false;
	char *match_path = NULL;	
	size_t name_size = 0;
	
	// read the list of match ids from the driver's configuration file
	if (NULL == (match_path = get_abs_path(base_path, name, MATCH_EXT))) {
		goto cleanup;
	}	
	
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
 * @param dir_path the path to the directory where we search for drivers. * 
 */ 
static int lookup_available_drivers(const char *dir_path)
{
	int drv_cnt = 0;
	DIR *dir = NULL;
	struct dirent *diren;

	dir = opendir(dir_path);
	if (dir != NULL) {
		driver_t *drv = create_driver();
		while ((diren = readdir(dir))) {			
			if (get_driver_info(dir_path, diren->d_name, drv)) {
				add_driver(drv);
				drv = create_driver();
			}	
		}
		delete_driver(drv);
		closedir(dir);
	}
	
	return drv_cnt;
}

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
	assert(node != NULL);
	
	node->parent = parent;
	if (parent != NULL) {
		list_append(&node->sibling, &parent->children);
	}
	
	list_initialize(&node->children);
	
	list_initialize(&node->match_ids.ids);	
}

static node_t * create_root_node()
{
	node_t *node = create_dev_node();
	
}

static void init_device_tree(dev_tree_t *tree)
{
	// create root node and add it to the device tree
	tree->root_node = create_root_node();
	

	// find suitable driver and start it
}

/** Initialize device manager internal structures.
 */
static bool devman_init()
{
	// initialize list of available drivers
	lookup_available_drivers(DRIVER_DEFAULT_STORE);

	// create root device node 
	init_device_tree(&device_tree);

	return true;
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Manager\n");

	if (!devman_init()) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}

	/*
	// Set a handler of incomming connections
	async_set_client_connection(devman_connection);

	// Register device manager at naming service
	ipcarg_t phonead;
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAN, 0, 0, &phonead) != 0)
		return -1;

	printf(NAME ": Accepting connections\n");
	async_manager();*/

	// Never reached
	return 0;
}
