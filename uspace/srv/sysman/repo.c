/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <adt/hash.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>

#include "repo.h"
#include "edge.h"
#include "log.h"

// TODO rename to repository (dynamic nature of units storage, and do not name it godlike Manager :-)

LIST_INITIALIZE(units);

static hash_table_t units_by_name;
static hash_table_t units_by_handle;

/* Hash table functions */
static size_t units_by_handle_ht_hash(const ht_link_t *item)
{
	unit_t *unit =
	    hash_table_get_inst(item, unit_t, units_by_handle);
	return unit->handle;
}

static size_t units_by_handle_ht_key_hash(void *key)
{
	return *(unit_handle_t *)key;
}

static bool units_by_handle_ht_equal(const ht_link_t *item1, const ht_link_t *item2)
{
	return
	    hash_table_get_inst(item1, unit_t, units_by_handle) ==
	    hash_table_get_inst(item2, unit_t, units_by_handle);
}

static bool units_by_handle_ht_key_equal(void *key, const ht_link_t *item)
{
	return *(unit_handle_t *)key ==
	    hash_table_get_inst(item, unit_t, units_by_handle)->handle;
}

static hash_table_ops_t units_by_handle_ht_ops = {
	.hash            = &units_by_handle_ht_hash,
	.key_hash        = &units_by_handle_ht_key_hash,
	.equal           = &units_by_handle_ht_equal,
	.key_equal       = &units_by_handle_ht_key_equal,
	.remove_callback = NULL // TODO realy unneeded?
};

static size_t units_by_name_ht_hash(const ht_link_t *item)
{
	unit_t *unit =
	    hash_table_get_inst(item, unit_t, units_by_name);
	return hash_string(unit->name);
}

static size_t units_by_name_ht_key_hash(void *key)
{
	return hash_string((const char *)key);
}

static bool units_by_name_ht_equal(const ht_link_t *item1, const ht_link_t *item2)
{
	return
	    hash_table_get_inst(item1, unit_t, units_by_handle) ==
	    hash_table_get_inst(item2, unit_t, units_by_handle);
}

static bool units_by_name_ht_key_equal(void *key, const ht_link_t *item)
{
	return str_cmp((const char *)key,
	    hash_table_get_inst(item, unit_t, units_by_name)->name) == 0;
}


static hash_table_ops_t units_by_name_ht_ops = {
	.hash            = &units_by_name_ht_hash,
	.key_hash        = &units_by_name_ht_key_hash,
	.equal           = &units_by_name_ht_equal,
	.key_equal       = &units_by_name_ht_key_equal,
	.remove_callback = NULL // TODO realy unneeded?
};

/* Repository functions */

void repo_init(void)
{
	hash_table_create(&units_by_name, 0, 0, &units_by_name_ht_ops);
	hash_table_create(&units_by_handle, 0, 0, &units_by_handle_ht_ops);
}

int repo_add_unit(unit_t *unit)
{
	assert(unit);
	assert(unit->state == STATE_EMBRYO);
	assert(unit->handle == 0);
	assert(unit->name != NULL);
	sysman_log(LVL_DEBUG2, "%s('%s')", __func__, unit_name(unit));

	if (hash_table_insert_unique(&units_by_name, &unit->units_by_name)) {
		/* Pointers are same size as unit_handle_t both on 32b and 64b */
		unit->handle = (unit_handle_t)unit;

		hash_table_insert(&units_by_handle, &unit->units_by_handle);
		list_append(&unit->units, &units);
		return EOK;
	} else {
		return EEXISTS;
	}
}

void repo_begin_update(void) {
	sysman_log(LVL_DEBUG2, "%s", __func__);
}

static bool repo_commit_unit(ht_link_t *ht_link, void *arg)
{
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units_by_name);
	if (unit->state == STATE_EMBRYO) {
		unit->state = STATE_STOPPED;
	}

	list_foreach(unit->edges_out, edges_out, unit_edge_t, e) {
		e->commited = true;
	}
	return true;
}

/** Marks newly added units_by_name as usable (via state change) */
void repo_commit(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	/*
	 * Apply commit to all units_by_name, each commited unit commits its outgoing
	 * deps, thus eventually commiting all embryo deps as well.
	 */
	hash_table_apply(&units_by_name, &repo_commit_unit, NULL);
}

static bool repo_rollback_unit(ht_link_t *ht_link, void *arg)
{
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units_by_name);

	list_foreach_safe(unit->edges_out, cur_link, next_link) {
		unit_edge_t *e =
		    list_get_instance(cur_link, unit_edge_t, edges_out);
		if (!e->commited) {
			edge_remove(&e);
		}
	}

	if (unit->state == STATE_EMBRYO) {
		hash_table_remove_item(&units_by_name, ht_link);
		list_remove(&unit->units);
		unit_destroy(&unit);
	}

	return true;
}

/** Remove all uncommited units_by_name and edges from configuratio
 *
 * Memory used by removed object is released.
 */
void repo_rollback(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	hash_table_apply(&units_by_name, &repo_rollback_unit, NULL);
}

static bool repo_resolve_unit(ht_link_t *ht_link, void *arg)
{
	bool *has_error_ptr = arg;
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units_by_name);

	list_foreach(unit->edges_out, edges_out, unit_edge_t, e) {
		assert(e->input == unit);
		assert((e->output != NULL) != (e->output_name != NULL));
		if (e->output) {
			continue;
		}

		unit_t *output =
		    repo_find_unit_by_name(e->output_name);
		if (output == NULL) {
			sysman_log(LVL_ERROR,
			    "Cannot resolve dependency of '%s' to unit '%s'",
			    unit_name(unit), e->output_name);
			*has_error_ptr = true;
			// TODO should we just leave the sprout untouched?
		} else {
			edge_resolve_output(e, output);
		}
	}

	return true;
}

/** Resolve unresolved dependencies between any pair of units_by_name
 *
 * @return EOK      on success
 * @return ENOENT  when one or more resolution fails, information is logged
 */
int repo_resolve_references(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	bool has_error = false;
	hash_table_apply(&units_by_name, &repo_resolve_unit, &has_error);

	return has_error ? ENOENT : EOK;
}

unit_t *repo_find_unit_by_name(const char *name)
{
	ht_link_t *ht_link = hash_table_find(&units_by_name, (void *)name);
	if (ht_link != NULL) {
		return hash_table_get_inst(ht_link, unit_t, units_by_name);
	} else {
		return NULL;
	}
}

unit_t *repo_find_unit_by_handle(unit_handle_t handle)
{
	ht_link_t *ht_link = hash_table_find(&units_by_handle, &handle);
	if (ht_link != NULL) {
		return hash_table_get_inst(ht_link, unit_t, units_by_handle);
	} else {
		return NULL;
	}
}

