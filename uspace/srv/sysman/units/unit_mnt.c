#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "log.h"
#include "unit.h"

static const char *section_name = "Mount";

static config_item_t unit_configuration[] = {
	{"What",  &config_parse_string, offsetof(unit_mnt_t, device),     NULL},
	{"Where", &config_parse_string, offsetof(unit_mnt_t, mountpoint), NULL},
	{"Type",  &config_parse_string, offsetof(unit_mnt_t, type),       NULL},
	CONFIGURATION_ITEM_SENTINEL
};

static void unit_mnt_init(unit_t *unit)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);

	u_mnt->type = NULL;
	u_mnt->mountpoint = NULL;
	u_mnt->device = NULL;
}

static void unit_mnt_destroy(unit_t *unit)
{
	assert(unit->type == UNIT_MOUNT);
	unit_mnt_t *u_mnt = CAST_MNT(unit);

	sysman_log(LVL_DEBUG2, "%s, %p, %p, %p", __func__,
	    u_mnt->type, u_mnt->mountpoint, u_mnt->device);
	free(u_mnt->type);
	free(u_mnt->mountpoint);
	free(u_mnt->device);
}

static int unit_mnt_load(unit_t *unit, ini_configuration_t *ini_conf,
    text_parse_t *text_parse)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);

	ini_section_t *section = ini_get_section(ini_conf, section_name);
	if (section == NULL) {
		sysman_log(LVL_ERROR,
		    "Expected section '%s' in configuration of unit '%s'",
		    section_name, unit_name(unit));
		return ENOENT;
	}

	return config_load_ini_section(unit_configuration, section, u_mnt,
	    text_parse);
}

static int unit_mnt_start(unit_t *unit)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);

	fibril_mutex_lock(&unit->state_mtx);
	
	// TODO think about unit's lifecycle (is STOPPED only acceptable?)
	assert(unit->state == STATE_STOPPED);
	unit->state = STATE_STARTING;
	
	fibril_condvar_broadcast(&unit->state_cv);
	fibril_mutex_unlock(&unit->state_mtx);


	// TODO use other mount parameters
	int rc = mount(u_mnt->type, u_mnt->mountpoint, u_mnt->device, "",
	    IPC_FLAG_BLOCKING, 0);

	if (rc == EOK) {
		sysman_log(LVL_NOTE, "Mount ('%s') mounted", unit_name(unit));
		unit_set_state(unit, STATE_STARTED);
	} else {
		sysman_log(LVL_ERROR, "Mount ('%s') failed (%i)",
		    unit_name(unit), rc);
		unit_set_state(unit, STATE_FAILED);
	}

	return rc;
}

DEFINE_UNIT_VMT(unit_mnt)

