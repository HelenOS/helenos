/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#include <loc.h>
#include <stdio.h>

#include "devman.h"
#include "fun.h"
#include "loc.h"

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

	loc_service_register(loc_pathname, &fun->service_id);

	tree_add_loc_function(tree, fun);

	free(loc_name);
	free(loc_pathname);
}

errno_t loc_unregister_tree_function(fun_node_t *fun, dev_tree_t *tree)
{
	errno_t rc = loc_service_unregister(fun->service_id);
	tree_rem_loc_function(tree, fun);
	return rc;
}

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

void tree_rem_loc_function(dev_tree_t *tree, fun_node_t *fun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));

	hash_table_remove(&tree->loc_functions, &fun->service_id);
}

/** @}
 */
