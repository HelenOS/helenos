#ifndef SYSMAN_DEP_H
#define SYSMAN_DEP_H

#include <adt/list.h>

#include "unit.h"

typedef enum {
	DEP_EMBRYO,
	DEP_VALID
} dependency_state_t;

/** Dependency edge between unit in dependency graph
 *
 * @code
 * dependant ---> dependency
 * @endcode
 *
 */
typedef struct {
	/** Link to dependants list */
	link_t dependants;
	/** Link to dependencies list */
	link_t dependencies;

	dependency_state_t state;

	/** Unit that depends on another */
	unit_t *dependant;

	/** Unit that is dependency for another */
	unit_t *dependency;

	/** Name of the dependency unit, for resolved dependencies it's NULL
	 *
	 * @note Either dependency or dependency_name is set. Never both nor
	 *       none.
	 */
	char *dependency_name;
} unit_dependency_t;

extern unit_dependency_t *dep_dependency_create(void);
extern void dep_dependency_destroy(unit_dependency_t **);

extern int dep_sprout_dependency(unit_t *, const char *);
extern void dep_resolve_dependency(unit_dependency_t *, unit_t *);

extern int dep_add_dependency(unit_t *, unit_t *);
extern void dep_remove_dependency(unit_dependency_t **);


#endif


