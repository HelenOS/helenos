#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "unit.h"

/** Virtual method table for each unit type */
static unit_ops_t *unit_type_vmts[] = {
	[UNIT_TARGET]        = &unit_tgt_ops,
	[UNIT_MOUNT]         = &unit_mnt_ops,
	[UNIT_CONFIGURATION] = &unit_cfg_ops
};

static void unit_init(unit_t *unit, unit_type_t type)
{
	assert(unit);

	memset(unit, 0, sizeof(unit_t));
	link_initialize(&unit->units);
	
	unit->type = type;
	unit->state = STATE_EMBRYO;
	fibril_mutex_initialize(&unit->state_mtx);
	fibril_condvar_initialize(&unit->state_cv);

	list_initialize(&unit->dependants);
	list_initialize(&unit->dependencies);

	unit_type_vmts[unit->type]->init(unit);
}

unit_t *unit_create(unit_type_t type)
{
	unit_t *unit = malloc(sizeof(unit_t));
	if (unit != NULL) {
		unit_init(unit, type);
	}
	return unit;
}

/** Release resources used by unit structure */
void unit_destroy(unit_t **unit_ptr)
{
	unit_t *unit = *unit_ptr;
	if (unit == NULL)
		return;

	unit_type_vmts[unit->type]->destroy(unit);
	/* TODO:
	 * 	edges,
	 * 	check it's not an active unit,
	 * 	other resources to come
	 */
	free(unit);
	unit_ptr = NULL;
}

void unit_set_state(unit_t *unit, unit_state_t state)
{
	fibril_mutex_lock(&unit->state_mtx);
	unit->state = state;
	fibril_condvar_broadcast(&unit->state_cv);
	fibril_mutex_unlock(&unit->state_mtx);
}

/** Issue request to restarter to start a unit
 *
 * Return from this function only means start request was issued.
 * If you need to wait for real start of the unit, use waiting on state_cv.
 */
int unit_start(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s(%p)", __func__, unit);
	return unit_type_vmts[unit->type]->start(unit);
}
