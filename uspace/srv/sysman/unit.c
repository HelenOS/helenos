#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>

#include "unit.h"

static void unit_init(unit_t *unit, unit_type_t type)
{
	assert(unit);

	link_initialize(&unit->units);
	
	unit->type = type;
	unit->state = STATE_EMBRYO;
	fibril_mutex_initialize(&unit->state_mtx);
	fibril_condvar_initialize(&unit->state_cv);

	list_initialize(&unit->dependants);
	list_initialize(&unit->dependencies);
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
void unit_destroy(unit_t **unit)
{
	if (*unit == NULL)
		return;
	/* TODO:
	 * 	edges,
	 * 	specific unit data,
	 * 	check it's not an active unit,
	 * 	other resources to come
	 */
	free(*unit);
	*unit = NULL;
}

/** Issue request to restarter to start a unit
 *
 * Return from this function only means start request was issued.
 * If you need to wait for real start of the unit, use waiting on state_cv.
 */
int unit_start(unit_t *unit)
{
	// TODO actually start the unit
	printf("Starting unit of type %i\n", unit->type);
	return EOK;
}
