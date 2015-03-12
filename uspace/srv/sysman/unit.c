#include <assert.h>
#include <mem.h>
#include <stdlib.h>

#include "unit.h"

static void unit_init(unit_t *unit, unit_type_t type)
{
	assert(unit);

	memset(unit, 0, sizeof(unit));

	link_initialize(&unit->units);
	list_initialize(&unit->dependants);
	list_initialize(&unit->dependencies);

	unit->type = type;
	unit->state = STATE_EMBRYO;
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
