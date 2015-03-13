#include <errno.h>
#include <stdlib.h>

#include "dep.h"

/**
 * @return        EOK on success
 * @return        ENOMEM
 */
int dep_add_dependency(unit_t *dependant, unit_t *dependency)
{
	unit_dependency_t *edge = malloc(sizeof(unit_dependency_t));
	if (edge == NULL) {
		return ENOMEM;
	}
	link_initialize(&edge->dependants);
	link_initialize(&edge->dependencies);

	// TODO check existence of the edge
	// TODO locking
	// TODO check types and states of connected units
	list_append(&edge->dependants, &dependency->dependants);
	list_append(&edge->dependencies, &dependant->dependencies);

	edge->dependant = dependant;
	edge->dependency = dependency;
	return EOK;
}
