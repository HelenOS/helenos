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

#include "configuration.h"
#include "dep.h"
#include "log.h"

static hash_table_t units;

/* Hash table functions */
static size_t units_ht_hash(const ht_link_t *item)
{
	unit_t *unit =
	    hash_table_get_inst(item, unit_t, units);
	return hash_string(unit->name);
}

static size_t units_ht_key_hash(void *key)
{
	return hash_string((const char *)key);
}

static bool units_ht_equal(const ht_link_t *item1, const ht_link_t *item2)
{
	return str_cmp(
	    hash_table_get_inst(item1, unit_t, units)->name,
	    hash_table_get_inst(item2, unit_t, units)->name) == 0;
}

static bool units_ht_key_equal(void *key, const ht_link_t *item)
{
	return str_cmp((const char *)key,
	    hash_table_get_inst(item, unit_t, units)->name) == 0;
}


static hash_table_ops_t units_ht_ops = {
	.hash            = &units_ht_hash,
	.key_hash        = &units_ht_key_hash,
	.equal           = &units_ht_equal,
	.key_equal       = &units_ht_key_equal,
	.remove_callback = NULL // TODO realy unneeded?
};

/* Configuration functions */

void configuration_init(void)
{
	hash_table_create(&units, 0, 0, &units_ht_ops);
}

int configuration_add_unit(unit_t *unit)
{
	assert(unit);
	assert(unit->state == STATE_EMBRYO);
	assert(unit->name != NULL);
	sysman_log(LVL_DEBUG2, "%s('%s')", __func__, unit_name(unit));

	if (hash_table_insert_unique(&units, &unit->units)) {
		return EOK;
	} else {
		return EEXISTS;
	}
}

void configuration_start_update(void) {
	sysman_log(LVL_DEBUG2, "%s", __func__);
}

static bool configuration_commit_unit(ht_link_t *ht_link, void *arg)
{
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units);
	// TODO state locking?
	if (unit->state == STATE_EMBRYO) {
		unit->state = STATE_STOPPED;
	}

	list_foreach(unit->dependencies, dependencies, unit_dependency_t, dep) {
		if (dep->state == DEP_EMBRYO) {
			dep->state = DEP_VALID;
		}
	}
	return true;
}

/** Marks newly added units as usable (via state change) */
void configuration_commit(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	/*
	 * Apply commit to all units, each commited unit commits its outgoing
	 * deps, thus eventually commiting all embryo deps as well.
	 */
	hash_table_apply(&units, &configuration_commit_unit, NULL);
}

static bool configuration_rollback_unit(ht_link_t *ht_link, void *arg)
{
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units);

	list_foreach_safe(unit->dependencies, cur_link, next_link) {
		unit_dependency_t *dep =
		    list_get_instance(cur_link, unit_dependency_t, dependencies);
		if (dep->state == DEP_EMBRYO) {
			dep_remove_dependency(&dep);
		}
	}

	if (unit->state == STATE_EMBRYO) {
		hash_table_remove_item(&units, ht_link);
		unit_destroy(&unit);
	}

	return true;
}

void configuration_rollback(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	hash_table_apply(&units, &configuration_rollback_unit, NULL);
}

static bool configuration_resolve_unit(ht_link_t *ht_link, void *arg)
{
	bool *has_error_ptr = arg;
	unit_t *unit = hash_table_get_inst(ht_link, unit_t, units);

	list_foreach(unit->dependencies, dependencies, unit_dependency_t, dep) {
		assert(dep->dependant == unit);
		assert((dep->dependency != NULL) != (dep->dependency_name != NULL));
		if (dep->dependency) {
			continue;
		}

		unit_t *dependency =
		    configuration_find_unit_by_name(dep->dependency_name);
		if (dependency == NULL) {
			sysman_log(LVL_ERROR,
			    "Cannot resolve dependency of '%s' to unit '%s'",
			    unit_name(unit), dep->dependency_name);
			*has_error_ptr = true;
			// TODO should we just leave the sprout untouched?
		} else {
			dep_resolve_dependency(dep, dependency);
		}
	}

	return true;
}

/** Resolve unresolved dependencies between any pair of units
 *
 * @return EOK      on success
 * @return ENONENT  when one or more resolution fails, information is logged
 */
int configuration_resolve_dependecies(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	bool has_error = false;
	hash_table_apply(&units, &configuration_resolve_unit, &has_error);

	return has_error ? ENOENT : EOK;
}

unit_t *configuration_find_unit_by_name(const char *name)
{
	ht_link_t *ht_link = hash_table_find(&units, (void *)name);
	if (ht_link != NULL) {
		return hash_table_get_inst(ht_link, unit_t, units);
	} else {
		return NULL;
	}
}

