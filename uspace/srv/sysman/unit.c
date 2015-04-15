#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <conf/configuration.h>
#include <conf/ini.h>
#include <mem.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#include "dep.h"
#include "log.h"
#include "unit.h"

/** Virtual method table for each unit type */
unit_vmt_t *unit_type_vmts[] = {
	[UNIT_TARGET]        = &unit_tgt_ops,
	[UNIT_MOUNT]         = &unit_mnt_ops,
	[UNIT_CONFIGURATION] = &unit_cfg_ops
};

static const char *section_name = "Unit";

static config_item_t unit_configuration[] = {
	{"After", &unit_parse_unit_list, 0, ""},
	CONFIGURATION_ITEM_SENTINEL
};


static void unit_init(unit_t *unit, unit_type_t type)
{
	assert(unit);

	// TODO is this necessary?
	memset(unit, 0, sizeof(unit_t));
	
	unit->type = type;
	unit->name = NULL;

	unit->state = STATE_EMBRYO;
	fibril_mutex_initialize(&unit->state_mtx);
	fibril_condvar_initialize(&unit->state_cv);

	list_initialize(&unit->dependants);
	list_initialize(&unit->dependencies);

	UNIT_VMT(unit)->init(unit);
}

unit_t *unit_create(unit_type_t type)
{
	size_t size = unit_type_vmts[type]->size;
	unit_t *unit = malloc(size);
	if (unit != NULL) {
		unit_init(unit, type);
	}
	return unit;
}

/** Release resources used by unit structure */
void unit_destroy(unit_t **unit_ptr)
{
	unit_t *unit = *unit_ptr;
	if (unit == NULL)
		return;

	UNIT_VMT(unit)->destroy(unit);
	/* TODO:
	 * 	edges,
	 * 	check it's not an active unit,
	 * 	other resources to come
	 */
	free(unit->name);
	free(unit);
	unit_ptr = NULL;
}

void unit_set_state(unit_t *unit, unit_state_t state)
{
	fibril_mutex_lock(&unit->state_mtx);
	unit->state = state;
	fibril_condvar_broadcast(&unit->state_cv);
	fibril_mutex_unlock(&unit->state_mtx);
}

/** Issue request to restarter to start a unit
 *
 * Return from this function only means start request was issued.
 * If you need to wait for real start of the unit, use waiting on state_cv.
 */
int unit_start(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));
	return UNIT_VMT(unit)->start(unit);
}

int unit_load(unit_t *unit, ini_configuration_t *ini_conf,
    text_parse_t *text_parse)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));

	int rc = EOK;
	ini_section_t *unit_section = ini_get_section(ini_conf, section_name);
	if (unit_section) {
		rc = config_load_ini_section(unit_configuration,
		    unit_section, unit, text_parse);
	}
				
	if (rc != EOK) {
		return rc;
	} else {
		return UNIT_VMT(unit)->load(unit, ini_conf, text_parse);
	}
}

unit_type_t unit_type_name_to_type(const char *type_name)
{
	if (str_cmp(type_name, "cfg") == 0)
		return UNIT_CONFIGURATION;

	else if (str_cmp(type_name, "mnt") == 0)
		return UNIT_MOUNT;

	else if (str_cmp(type_name, "tgt") == 0)
		return UNIT_TARGET;

	else
		return UNIT_TYPE_INVALID;
}

/** Format unit name to be presented to user */
const char *unit_name(const unit_t *unit)
{
	return unit->name ? unit->name : "";
}




bool unit_parse_unit_list(const char *value, void *dst, text_parse_t *parse,
    size_t lineno)
{
	unit_t *unit = dst;
	bool result;
	char *my_value = str_dup(value);

	if (!my_value) {
		result = false;
		goto finish;
	}

	char *to_split = my_value;
	char *cur_tok;

	while ((cur_tok = str_tok(to_split, " ", &to_split))) {
		if (dep_sprout_dependency(unit, cur_tok) != EOK) {
			result = false;
			goto finish;
		}
	}

	result = true;

finish:
	free(my_value);
	return result;
}
