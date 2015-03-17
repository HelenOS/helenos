#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "log.h"
#include "unit.h"
#include "unit_mnt.h"

static void unit_mnt_init(unit_t *unit)
{
	assert(unit->data.mnt.type == NULL);
	assert(unit->data.mnt.mountpoint == NULL);
	assert(unit->data.mnt.device == NULL);
}

static int unit_mnt_start(unit_t *unit)
{
	fibril_mutex_lock(&unit->state_mtx);
	
	// TODO think about unit's lifecycle (is STOPPED only acceptable?)
	assert(unit->state == STATE_STOPPED);
	unit->state = STATE_STARTING;
	
	fibril_condvar_broadcast(&unit->state_cv);
	fibril_mutex_unlock(&unit->state_mtx);


	unit_mnt_t *data = &unit->data.mnt;

	// TODO use other mount parameters
	int rc = mount(data->type, data->mountpoint, data->device, "",
	    IPC_FLAG_BLOCKING, 0);

	if (rc == EOK) {
		sysman_log(LVL_NOTE, "Mount (%p) mounted", unit);
		unit_set_state(unit, STATE_STARTED);
	} else {
		sysman_log(LVL_ERROR, "Mount (%p) failed (%i)", unit, rc);
		unit_set_state(unit, STATE_FAILED);
	}

	return rc;
}

static void unit_mnt_destroy(unit_t *unit)
{
	free(unit->data.mnt.type);
	free(unit->data.mnt.mountpoint);
	free(unit->data.mnt.device);
}


DEFINE_UNIT_OPS(unit_mnt)

