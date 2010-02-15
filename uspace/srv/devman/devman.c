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

#include "devman.h"
#include "util.h"


driver_t * create_driver() 
{
	printf(NAME ": create_driver\n");
	
	driver_t *res = malloc(sizeof(driver_t));
	if(res != NULL) {
		init_driver(res);
	}
	return res;
}

char * read_id(const char **buf) 
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
 */ 
int lookup_available_drivers(link_t *drivers_list, const char *dir_path)
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

node_t * create_root_node()
{
	node_t *node = create_dev_node();
	if (node) {
		init_dev_node(node, NULL);
		match_id_t *id = create_match_id();
		id->id = "root";
		id->score = 100;
		add_match_id(&node->match_ids, id);
	}
	return node;	
}

driver_t * find_best_match_driver(link_t *drivers_list, node_t *node)
{
	driver_t *best_drv = NULL, *drv = NULL;
	int best_score = 0, score = 0;
	link_t *link = drivers_list->next;	
	
	while (link != drivers_list) {
		drv = list_get_instance(link, driver_t, drivers);
		score = get_match_score(drv, node);
		if (score > best_score) {
			best_score = score;
			best_drv = drv;
		}		
	}	
	
	return best_drv;	
}

void attach_driver(node_t *node, driver_t *drv) 
{
	node->drv = drv;
	list_append(&node->driver_devices, &drv->devices);
}

bool start_driver(driver_t *drv)
{
	char *argv[2];
	
	printf(NAME ": spawning driver %s\n", drv->name);
	
	argv[0] = drv->name;
	argv[1] = NULL;
	
	if (!task_spawn(drv->binary_path, argv)) {
		printf(NAME ": error spawning %s\n", drv->name);
		return false;
	}
	
	return true;
}

bool add_device(driver_t *drv, node_t *node)
{
	// TODO
	
	// pass a new device to the running driver, which was previously assigned to it
		// send the phone of the parent's driver and device's handle within the parent's driver to the driver 
		// let the driver to probe the device and specify whether the device is actually present
		// if the device is present, remember its handle within the driver
	
	return true;
}

bool assign_driver(node_t *node, link_t *drivers_list) 
{
	// find the driver which is the most suitable for handling this device
	driver_t *drv = find_best_match_driver(drivers_list, node);
	if (NULL == drv) {
		return false;		
	}
	
	// attach the driver to the device
	attach_driver(node, drv);
	
	if (!drv->running) {
		// start driver
		start_driver(drv);
	} else {
		// notify driver about new device
		add_device(drv, node);		
	}
	
	return true;
}

bool init_device_tree(dev_tree_t *tree, link_t *drivers_list)
{
	printf(NAME ": init_device_tree.");
	// create root node and add it to the device tree
	if (NULL == (tree->root_node = create_root_node())) {
		return false;
	}

	// find suitable driver and start it
	return assign_driver(tree->root_node, drivers_list);
}

