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
	
	atomic_set(&dev->refcnt, 0);
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
	atomic_inc(&dev->refcnt);
}

/** Decrease device node reference count.
 *
 * When the count drops to zero the device node is freed.
 *
 * @param dev	Device node
 */
void dev_del_ref(dev_node_t *dev)
{
	if (atomic_predec(&dev->refcnt) == 0)
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
