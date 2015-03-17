#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>

#include "configuration.h"
#include "log.h"

static list_t units;
static fibril_mutex_t units_mtx;

void configuration_init(void)
{
	list_initialize(&units);
	fibril_mutex_initialize(&units_mtx);
}

int configuration_add_unit(unit_t *unit)
{
	sysman_log(LVL_DEBUG2, "%s(%p)", __func__, unit);
	assert(unit);
	assert(unit->state == STATE_EMBRYO);

	fibril_mutex_lock(&units_mtx);
	list_append(&unit->units, &units);
	
	// TODO check name uniqueness
	fibril_mutex_unlock(&units_mtx);
	return EOK;
}

/** Marks newly added units as usable (via state change) */
int configuration_commit(void)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	fibril_mutex_lock(&units_mtx);
	list_foreach(units, units, unit_t, u) {
		if (u->state == STATE_EMBRYO) {
			u->state = STATE_STOPPED;
		}
	}
	fibril_mutex_unlock(&units_mtx);

	return EOK;
}
