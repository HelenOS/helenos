#ifndef SYSMAN_DEP_H
#define SYSMAN_DEP_H

#include <adt/list.h>

#include "unit.h"

/** Dependency edge between unit in dependency graph */
typedef struct {
	link_t dependants;
	link_t dependencies;

	/** Unit that depends on another */
	unit_t *dependant;
	/** Unit that is dependency for another */
	unit_t *dependency;
} unit_dependency_t;

extern int dep_add_dependency(unit_t *, unit_t *);

#endif


