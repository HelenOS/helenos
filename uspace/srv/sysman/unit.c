/*
 * Copyright (c) 2015 Michal Koutny
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
#include <sysman/unit.h>

#include "edge.h"
#include "log.h"
#include "sysman.h"
#include "unit.h"

/** Virtual method table for each unit type */
unit_vmt_t *unit_type_vmts[] = {
	[UNIT_CONFIGURATION] = &unit_cfg_vmt,
	[UNIT_MOUNT]         = &unit_mnt_vmt,
	[UNIT_TARGET]        = &unit_tgt_vmt,
	[UNIT_SERVICE]       = &unit_svc_vmt
};

static const char *section_name = "Unit";

static config_item_t unit_configuration[] = {
	{"After", &unit_parse_unit_list, 0, ""},
	CONFIGURATION_ITEM_SENTINEL
};


static void unit_init(unit_t *unit, unit_type_t type)
{
	assert(unit);

	size_t size = unit_type_vmts[type]->size;
	memset(unit, 0, size);
	
	unit->type = type;
	unit->state = STATE_STOPPED;
	unit->repo_state = REPO_EMBRYO;

	link_initialize(&unit->units);
	link_initialize(&unit->bfs_link);
	list_initialize(&unit->edges_in);
	list_initialize(&unit->edges_out);

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
	 * 	edges
	 */
	free(unit->name);
	free(unit);
	unit_ptr = NULL;
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

/** Issue request to restarter to start a unit
 *
 * Ideally this function is non-blocking synchronous, however, some units
 * cannot be started synchronously and thus return from this function generally
 * means that start was requested.
 *
 * Check state of the unit for actual result, start method can end in states:
 *   - STATE_STARTED, (succesful synchronous start)
 *   - STATE_STARTING, (succesful asynchronous start request)
 *   - STATE_FAILED.  (unit state changed and error occured)
 */
int unit_start(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));
	return UNIT_VMT(unit)->start(unit);
}

/** Issue request to restarter to stop a unit
 *
 * Same semantics like for unit_start applies.
 */
int unit_stop(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));
	return UNIT_VMT(unit)->stop(unit);
}

void unit_exposee_created(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));
	return UNIT_VMT(unit)->exposee_created(unit);
}

void unit_fail(unit_t *unit)
{
	sysman_log(LVL_DEBUG, "%s('%s')", __func__, unit_name(unit));
	return UNIT_VMT(unit)->fail(unit);
}

void unit_notify_state(unit_t *unit)
{
	sysman_raise_event(&sysman_event_unit_state_changed, unit);
}

// TODO move to libsysman
unit_type_t unit_type_name_to_type(const char *type_name)
{
	if (str_cmp(type_name, UNIT_CFG_TYPE_NAME) == 0)
		return UNIT_CONFIGURATION;

	else if (str_cmp(type_name, UNIT_MNT_TYPE_NAME) == 0)
		return UNIT_MOUNT;

	else if (str_cmp(type_name, UNIT_TGT_TYPE_NAME) == 0)
		return UNIT_TARGET;

	else if (str_cmp(type_name, UNIT_SVC_TYPE_NAME) == 0)
		return UNIT_SERVICE;

	else
		return UNIT_TYPE_INVALID;
}

/** Format unit name to be presented to user */
const char *unit_name(const unit_t *unit)
{
	return unit->name ? unit->name : "";
}

bool unit_parse_unit_list(const char *string, void *dst, text_parse_t *parse,
    size_t lineno)
{
	unit_t *unit = dst;
	bool result;
	char *my_string = str_dup(string);

	if (!my_string) {
		result = false;
		goto finish;
	}

	char *to_split = my_string;
	char *cur_tok;

	while ((cur_tok = str_tok(to_split, " ", &to_split))) {
		if (edge_sprout_out(unit, cur_tok) != EOK) {
			result = false;
			goto finish;
		}
	}

	result = true;

finish:
	free(my_string);
	return result;
}
