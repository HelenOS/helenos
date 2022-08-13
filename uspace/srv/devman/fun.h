/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef FUN_H_
#define FUN_H_

#include "devman.h"

extern fun_node_t *create_fun_node(void);
extern void delete_fun_node(fun_node_t *);
extern void fun_add_ref(fun_node_t *);
extern void fun_del_ref(fun_node_t *);
extern void fun_busy_lock(fun_node_t *);
extern void fun_busy_unlock(fun_node_t *);
extern fun_node_t *find_fun_node_no_lock(dev_tree_t *, devman_handle_t);
extern fun_node_t *find_fun_node(dev_tree_t *, devman_handle_t);
extern fun_node_t *find_fun_node_by_path(dev_tree_t *, char *);
extern fun_node_t *find_fun_node_in_device(dev_tree_t *tree, dev_node_t *,
    const char *);
extern bool set_fun_path(dev_tree_t *, fun_node_t *, fun_node_t *);
extern errno_t fun_online(fun_node_t *);
extern errno_t fun_offline(fun_node_t *);

#endif

/** @}
 */
