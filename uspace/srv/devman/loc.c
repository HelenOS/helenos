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
