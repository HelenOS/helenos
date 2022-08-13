/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#include <errno.h>

#include "dev.h"
#include "devman.h"

/** Create a new device node.
 *
 * @return		A device node structure.
 */
dev_node_t *create_dev_node(void)
{
	dev_node_t *dev;

	dev = calloc(1, sizeof(dev_node_t));
	if (dev == NULL)
		return NULL;

	refcount_init(&dev->refcnt);
	list_initialize(&dev->functions);
	link_initialize(&dev->driver_devices);

	return dev;
}

/** Delete a device node.
 *
 * @param node		The device node structure.
 */
void delete_dev_node(dev_node_t *dev)
{
	assert(list_empty(&dev->functions));
	assert(dev->pfun == NULL);
	assert(dev->drv == NULL);

	free(dev);
}

/** Increase device node reference count.
 *
 * @param dev	Device node
 */
void dev_add_ref(dev_node_t *dev)
{
	refcount_up(&dev->refcnt);
}

/** Decrease device node reference count.
 *
 * When the count drops to zero the device node is freed.
 *
 * @param dev	Device node
 */
void dev_del_ref(dev_node_t *dev)
{
	if (refcount_down(&dev->refcnt))
		delete_dev_node(dev);
}

/** Find the device node structure of the device witch has the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
dev_node_t *find_dev_node_no_lock(dev_tree_t *tree, devman_handle_t handle)
{
	assert(fibril_rwlock_is_locked(&tree->rwlock));

	ht_link_t *link = hash_table_find(&tree->devman_devices, &handle);
	if (link == NULL)
		return NULL;

	return hash_table_get_inst(link, dev_node_t, devman_dev);
}

/** Find the device node structure of the device witch has the specified handle.
 *
 * @param tree		The device tree where we look for the device node.
 * @param handle	The handle of the device.
 * @return		The device node.
 */
dev_node_t *find_dev_node(dev_tree_t *tree, devman_handle_t handle)
{
	dev_node_t *dev = NULL;

	fibril_rwlock_read_lock(&tree->rwlock);
	dev = find_dev_node_no_lock(tree, handle);
	if (dev != NULL)
		dev_add_ref(dev);

	fibril_rwlock_read_unlock(&tree->rwlock);

	return dev;
}

/** Get list of device functions. */
errno_t dev_get_functions(dev_tree_t *tree, dev_node_t *dev,
    devman_handle_t *hdl_buf, size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	assert(fibril_rwlock_is_locked(&tree->rwlock));

	buf_cnt = buf_size / sizeof(devman_handle_t);

	act_cnt = list_count(&dev->functions);
	*act_size = act_cnt * sizeof(devman_handle_t);

	if (buf_size % sizeof(devman_handle_t) != 0)
		return EINVAL;

	size_t pos = 0;
	list_foreach(dev->functions, dev_functions, fun_node_t, fun) {
		if (pos < buf_cnt) {
			hdl_buf[pos] = fun->handle;
		}

		pos++;
	}

	return EOK;
}

/** @}
 */
