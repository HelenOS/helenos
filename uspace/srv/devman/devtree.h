/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef DEVTREE_H_
#define DEVTREE_H_

#include <stdbool.h>

#include "devman.h"

extern bool init_device_tree(dev_tree_t *, driver_list_t *);
extern bool create_root_nodes(dev_tree_t *);
extern bool insert_dev_node(dev_tree_t *, dev_node_t *, fun_node_t *);
extern void remove_dev_node(dev_tree_t *, dev_node_t *);
extern bool insert_fun_node(dev_tree_t *, fun_node_t *, char *, dev_node_t *);
extern void remove_fun_node(dev_tree_t *, fun_node_t *);

#endif

/** @}
 */
