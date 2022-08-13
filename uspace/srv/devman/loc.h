/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef LOC_H_
#define LOC_H_

#include <ipc/loc.h>
#include "devman.h"

extern void loc_register_tree_function(fun_node_t *, dev_tree_t *);
extern errno_t loc_unregister_tree_function(fun_node_t *, dev_tree_t *);
extern fun_node_t *find_loc_tree_function(dev_tree_t *, service_id_t);
extern void tree_add_loc_function(dev_tree_t *, fun_node_t *);
extern void tree_rem_loc_function(dev_tree_t *, fun_node_t *);

#endif

/** @}
 */
