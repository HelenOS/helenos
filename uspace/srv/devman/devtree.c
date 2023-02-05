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

#include "dev.h"
#include "devtree.h"
#include "devman.h"
#include "driver.h"
#include "fun.h"

/* hash table operations */

static inline size_t handle_key_hash(const void *key)
{
	const devman_handle_t *handle = key;
	return *handle;
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

static bool devman_devices_key_equal(const void *key, const ht_link_t *item)
{
	const devman_handle_t *handle = key;
	dev_node_t *dev = hash_table_get_inst(item, dev_node_t, devman_dev);
	return dev->handle == *handle;
}

static bool devman_functions_key_equal(const void *key, const ht_link_t *item)
{
	const devman_handle_t *handle = key;
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, devman_fun);
	return fun->handle == *handle;
}

static inline size_t service_id_key_hash(const void *key)
{
	const service_id_t *service_id = key;
	return *service_id;
}

static size_t loc_functions_hash(const ht_link_t *item)
{
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, loc_fun);
	return service_id_key_hash(&fun->service_id);
}

static bool loc_functions_key_equal(const void *key, const ht_link_t *item)
{
	const service_id_t *service_id = key;
	fun_node_t *fun = hash_table_get_inst(item, fun_node_t, loc_fun);
	return fun->service_id == *service_id;
}

static const hash_table_ops_t devman_devices_ops = {
	.hash = devman_devices_hash,
	.key_hash = handle_key_hash,
	.key_equal = devman_devices_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static const hash_table_ops_t devman_functions_ops = {
	.hash = devman_functions_hash,
	.key_hash = handle_key_hash,
	.key_equal = devman_functions_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static const hash_table_ops_t loc_devices_ops = {
	.hash = loc_functions_hash,
	.key_hash = service_id_key_hash,
	.key_equal = loc_functions_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

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

	if (!insert_fun_node(tree, fun, str_dup(""), NULL)) {
		fun_del_ref(fun);	/* fun is destroyed */
		fibril_rwlock_write_unlock(&tree->rwlock);
		return false;
	}

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

	insert_dev_node(tree, dev, fun);

	fibril_rwlock_write_unlock(&tree->rwlock);

	return dev != NULL;
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
	bool rc = assign_driver(rdev, drivers_list, tree);
	dev_del_ref(rdev);

	return rc;
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

/** @}
 */
