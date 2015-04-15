#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>

#include "dep.h"

static void dep_dependency_init(unit_dependency_t *dep)
{
	link_initialize(&dep->dependants);
	link_initialize(&dep->dependencies);

	dep->dependency_name = NULL;
	dep->state = DEP_EMBRYO;
}

unit_dependency_t *dep_dependency_create(void)
{
	unit_dependency_t *dep = malloc(sizeof(unit_dependency_t));
	if (dep) {
		dep_dependency_init(dep);
	}
	return dep;
}

void dep_dependency_destroy(unit_dependency_t **dep_ptr)
{
	unit_dependency_t *dep = *dep_ptr;
	if (dep == NULL) {
		return;
	}

	list_remove(&dep->dependencies);
	list_remove(&dep->dependants);

	free(dep->dependency_name);
	free(dep);

	*dep_ptr = NULL;
}

int dep_sprout_dependency(unit_t *dependant, const char *dependency_name)
{
	unit_dependency_t *dep = dep_dependency_create();
	int rc;

	if (dep == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	dep->dependency_name = str_dup(dependency_name);
	if (dep->dependency_name == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	list_append(&dep->dependencies, &dependant->dependencies);
	dep->dependant = dependant;

	rc = EOK;

finish:
	if (rc != EOK) {
		dep_dependency_destroy(&dep);
	}
	return rc;
}

void dep_resolve_dependency(unit_dependency_t *dep, unit_t *unit)
{
	assert(dep->dependency == NULL);
	assert(dep->dependency_name != NULL);

	// TODO add to other side dependants list
	dep->dependency = unit;
	free(dep->dependency_name);
	dep->dependency_name = NULL;
}


/**
 * @return        EOK on success
 * @return        ENOMEM
 */
int dep_add_dependency(unit_t *dependant, unit_t *dependency)
{
	unit_dependency_t *dep = dep_dependency_create();
	if (dep == NULL) {
		return ENOMEM;
	}

	// TODO check existence of the dep
	// TODO locking
	// TODO check types and states of connected units
	list_append(&dep->dependants, &dependency->dependants);
	list_append(&dep->dependencies, &dependant->dependencies);

	dep->dependant = dependant;
	dep->dependency = dependency;
	return EOK;
}

/** Remove dependency from dependency graph
 *
 * Given dependency is removed from graph and unallocated.
 */
void dep_remove_dependency(unit_dependency_t **dep_ptr)
{
	// TODO here should be some checks, othewise replace this wrapper with
	//      direct destroy
	dep_dependency_destroy(dep_ptr);
}
