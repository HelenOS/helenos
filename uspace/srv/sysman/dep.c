#include <errno.h>
#include <stdlib.h>

#include "dep.h"

/**
 * @return        EOK on success
 * @return        ENOMEM
 */
int dep_add_dependency(unit_t *dependant, unit_t *dependency)
{
	unit_dependency_t *edge = malloc(sizeof(unit_t));
	if (edge == NULL) {
		return ENOMEM;
	}
	// TODO check existence of the edge
	// TODO locking
	// TODO check types and states of connected units
	/* Do not initalize links as they are immediately inserted into list */
	list_append(&edge->dependants, &dependency->dependants);
	list_append(&edge->dependencies, &dependant->dependencies);

	edge->dependant = dependant;
	edge->dependency = dependency;
	return EOK;
}
