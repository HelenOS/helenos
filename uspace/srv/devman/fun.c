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
#include <io/log.h>
#include <loc.h>

#include "dev.h"
#include "devman.h"
#include "devtree.h"
#include "driver.h"
#include "fun.h"
#include "main.h"
#include "loc.h"

static fun_node_t *find_node_child(dev_tree_t *, fun_node_t *, const char *);

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
	refcount_init(&fun->refcnt);
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
	refcount_up(&fun->refcnt);
}

/** Decrease function node reference count.
 *
 * When the count drops to zero the function node is freed.
 *
 * @param fun	Function node
 */
void fun_del_ref(fun_node_t *fun)
{
	if (refcount_down(&fun->refcnt))
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
bool set_fun_path(dev_tree_t *tree, fun_node_t *fun, fun_node_t *parent)
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

	list_foreach(dev->functions, dev_functions, fun_node_t, fun) {
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

static errno_t assign_driver_fibril(void *arg)
{
	dev_node_t *dev_node = (dev_node_t *) arg;
	assign_driver(dev_node, &drivers_list, &device_tree);

	/* Delete one reference we got from the caller. */
	dev_del_ref(dev_node);
	return EOK;
}

errno_t fun_online(fun_node_t *fun)
{
	dev_node_t *dev;

	fibril_rwlock_write_lock(&device_tree.rwlock);

	if (fun->state == FUN_ON_LINE) {
		fibril_rwlock_write_unlock(&device_tree.rwlock);
		log_msg(LOG_DEFAULT, LVL_WARN, "Function %s is already on line.",
		    fun->pathname);
		return EOK;
	}

	if (fun->ftype == fun_inner) {
		dev = create_dev_node();
		if (dev == NULL) {
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			return ENOMEM;
		}

		insert_dev_node(&device_tree, dev, fun);
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_add_function(fun=\"%s\")", fun->pathname);

	if (fun->ftype == fun_inner) {
		dev = fun->child;
		assert(dev != NULL);

		/* Give one reference over to assign_driver_fibril(). */
		dev_add_ref(dev);

		/*
		 * Try to find a suitable driver and assign it to the device.  We do
		 * not want to block the current fibril that is used for processing
		 * incoming calls: we will launch a separate fibril to handle the
		 * driver assigning. That is because assign_driver can actually include
		 * task spawning which could take some time.
		 */
		fid_t assign_fibril = fibril_create(assign_driver_fibril, dev);
		if (assign_fibril == 0) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to create fibril for "
			    "assigning driver.");
			/* XXX Cleanup */
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			return ENOMEM;
		}
		fibril_add_ready(assign_fibril);
	} else
		loc_register_tree_function(fun, &device_tree);

	fun->state = FUN_ON_LINE;
	fibril_rwlock_write_unlock(&device_tree.rwlock);

	return EOK;
}

errno_t fun_offline(fun_node_t *fun)
{
	errno_t rc;

	fibril_rwlock_write_lock(&device_tree.rwlock);

	if (fun->state == FUN_OFF_LINE) {
		fibril_rwlock_write_unlock(&device_tree.rwlock);
		log_msg(LOG_DEFAULT, LVL_WARN, "Function %s is already off line.",
		    fun->pathname);
		return EOK;
	}

	if (fun->ftype == fun_inner) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Offlining inner function %s.",
		    fun->pathname);

		if (fun->child != NULL) {
			dev_node_t *dev = fun->child;
			device_state_t dev_state;

			dev_add_ref(dev);
			dev_state = dev->state;

			fibril_rwlock_write_unlock(&device_tree.rwlock);

			/* If device is owned by driver, ask driver to give it up. */
			if (dev_state == DEVICE_USABLE) {
				rc = driver_dev_remove(&device_tree, dev);
				if (rc != EOK) {
					dev_del_ref(dev);
					return ENOTSUP;
				}
			}

			/* Verify that driver removed all functions */
			fibril_rwlock_read_lock(&device_tree.rwlock);
			if (!list_empty(&dev->functions)) {
				fibril_rwlock_read_unlock(&device_tree.rwlock);
				dev_del_ref(dev);
				return EIO;
			}

			driver_t *driver = dev->drv;
			fibril_rwlock_read_unlock(&device_tree.rwlock);

			if (driver)
				detach_driver(&device_tree, dev);

			fibril_rwlock_write_lock(&device_tree.rwlock);
			remove_dev_node(&device_tree, dev);

			/* Delete ref created when node was inserted */
			dev_del_ref(dev);
			/* Delete ref created by dev_add_ref(dev) above */
			dev_del_ref(dev);
		}
	} else {
		/* Unregister from location service */
		rc = loc_unregister_tree_function(fun, &device_tree);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed unregistering tree service.");
			return EIO;
		}

		fun->service_id = 0;
	}

	fun->state = FUN_OFF_LINE;
	fibril_rwlock_write_unlock(&device_tree.rwlock);

	return EOK;
}

/** @}
 */
