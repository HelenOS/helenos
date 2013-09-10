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

#include "dev.h"
#include "devman.h"
#include "driver.h"

static fun_node_t *find_node_child(dev_tree_t *, fun_node_t *, const char *);

/* hash table operations */

static inline size_t handle_key_hash(void *key)
{
	devman_handle_t handle = *(devman_handle_t*)key;
	return handle;
}

static size_t devman_devices_hash(const ht_link_t *item)
{
	dev_node_t *dev = hash_table_get_inst(item, dev_node_t, devman_dev);
	return handle_key_hash(&dev->handle);
}

static size_t devman_functions_hash(const ht_link_t *item)
{
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, devman_fun);
	return handle_key_hash(&fun->handle);
}

static bool devman_devices_key_equal(void *key, const ht_link_t *item)
{
	devman_handle_t handle = *(devman_handle_t*)key;
	dev_node_t *dev = hash_table_get_inst(item, dev_node_t, devman_dev);
	return dev->handle == handle;
}

static bool devman_functions_key_equal(void *key, const ht_link_t *item)
{
	devman_handle_t handle = *(devman_handle_t*)key;
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, devman_fun);
	return fun->handle == handle;
}

static inline size_t service_id_key_hash(void *key)
{
	service_id_t service_id = *(service_id_t*)key;
	return service_id;
}

static size_t loc_functions_hash(const ht_link_t *item)
{
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, loc_fun);
	return service_id_key_hash(&fun->service_id);
}

static bool loc_functions_key_equal(void *key, const ht_link_t *item)
{
	service_id_t service_id = *(service_id_t*)key;
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, loc_fun);
	return fun->service_id == service_id;
}


static hash_table_ops_t devman_devices_ops = {
	.hash = devman_devices_hash,
	.key_hash = handle_key_hash,
	.key_equal = devman_devices_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static hash_table_ops_t devman_functions_ops = {
	.hash = devman_functions_hash,
	.key_hash = handle_key_hash,
	.key_equal = devman_functions_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static hash_table_ops_t loc_devices_ops = {
	.hash = loc_functions_hash,
	.key_hash = service_id_key_hash,
	.key_equal = loc_functions_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "read_match_ids(conf_path=\"%s\")", conf_path);
	
	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len = 0;
	
	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to open `%s' for reading: %s.",
		    conf_path, str_error(fd));
		goto cleanup;
	}
	opened = true;
	
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (len == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Configuration file '%s' is empty.",
		    conf_path);
		goto cleanup;
	}
	
	buf = malloc(len + 1);
	if (buf == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Memory allocation failed when parsing file "
		    "'%s'.", conf_path);
		goto cleanup;
	}
	
	ssize_t read_bytes = read_all(fd, buf, len);
	if (read_bytes <= 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to read file '%s' (%zd).", conf_path,
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

/** Create root device and function node in the device tree.
 *
 * @param tree	The device tree.
 * @return	True on success, false otherwise.
 */
bool create_root_nodes(dev_tree_t *tree)
{
	fun_node_t *fun;
	dev_node_t *dev;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "create_root_nodes()");
	
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "init_device_tree()");
	
	tree->current_handle = 0;
	
	hash_table_create(&tree->devman_devices, 0, 0, &devman_devices_ops);
	hash_table_create(&tree->devman_functions, 0, 0, &devman_functions_ops);
	hash_table_create(&tree->loc_functions, 0, 0, &loc_devices_ops);
	
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
	fibril_mutex_initialize(&fun->busy_lock);
	link_initialize(&fun->dev_functions);
	list_initialize(&fun->match_ids.ids);
	
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
	free(fun->name);
	free(fun->pathname);
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

/** Make function busy for reconfiguration operations. */
void fun_busy_lock(fun_node_t *fun)
{
	fibril_mutex_lock(&fun->busy_lock);
}

/** Mark end of reconfiguration operation. */
void fun_busy_unlock(fun_node_t *fun)
{
	fibril_mutex_unlock(&fun->busy_lock);
}

/** Find the function node with the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the function.
 * @return		The function node.
 */
fun_node_t *find_fun_node_no_lock(dev_tree_t *tree, devman_handle_t handle)
{
	fun_node_t *fun;
	
	assert(fibril_rwlock_is_locked(&tree->rwlock));
	
	ht_link_t *link = hash_table_find(&tree->devman_functions, &handle);
	if (link == NULL)
		return NULL;
	
	fun = hash_table_get_inst(link, fun_node_t, devman_fun);
	
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
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to allocate device path.");
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
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "insert_dev_node(dev=%p, pfun=%p [\"%s\"])",
	    dev, pfun, pfun->pathname);

	/* Add the node to the handle-to-node map. */
	dev->handle = ++tree->current_handle;
	hash_table_insert(&tree->devman_devices, &dev->devman_dev);

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
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "remove_dev_node(dev=%p)", dev);
	
	/* Remove node from the handle-to-node map. */
	hash_table_remove(&tree->devman_devices, &dev->handle);
	
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
	hash_table_insert(&tree->devman_functions, &fun->devman_fun);

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
	hash_table_remove(&tree->devman_functions, &fun->handle);
	
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
	
	fibril_rwlock_read_lock(&tree->rwlock);
	ht_link_t *link = hash_table_find(&tree->loc_functions, &service_id);
	if (link != NULL) {
		fun = hash_table_get_inst(link, fun_node_t, loc_fun);
		fun_add_ref(fun);
	}
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return fun;
}

void tree_add_loc_function(dev_tree_t *tree, fun_node_t *fun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	hash_table_insert(&tree->loc_functions, &fun->loc_fun);
}

/** @}
 */
