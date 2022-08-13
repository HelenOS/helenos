/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef DEV_H_
#define DEV_H_

#include "devman.h"

extern dev_node_t *create_dev_node(void);
extern void delete_dev_node(dev_node_t *node);
extern void dev_add_ref(dev_node_t *);
extern void dev_del_ref(dev_node_t *);

extern dev_node_t *find_dev_node_no_lock(dev_tree_t *tree,
    devman_handle_t handle);
extern dev_node_t *find_dev_node(dev_tree_t *tree, devman_handle_t handle);
extern errno_t dev_get_functions(dev_tree_t *tree, dev_node_t *, devman_handle_t *,
    size_t, size_t *);

#endif

/** @}
 */
