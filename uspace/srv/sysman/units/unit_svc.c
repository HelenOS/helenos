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
#include <io/console.h> // temporary
#include <stdlib.h>
#include <task.h>

#include "log.h"
#include "unit.h"

static const char *section_name = "Service";

static config_item_t unit_configuration[] = {
	{"ExecStart", &util_parse_command, offsetof(unit_svc_t, exec_start), NULL},
	CONFIGURATION_ITEM_SENTINEL
};

static void unit_svc_init(unit_t *unit)
{
	unit_svc_t *u_svc = CAST_SVC(unit);
	assert(u_svc);
	util_command_init(&u_svc->exec_start);
}

static void unit_svc_destroy(unit_t *unit)
{
	assert(unit->type == UNIT_SERVICE);
	unit_svc_t *u_svc = CAST_SVC(unit);

	util_command_deinit(&u_svc->exec_start);
}

static int unit_svc_load(unit_t *unit, ini_configuration_t *ini_conf,
    text_parse_t *text_parse)
{
	unit_svc_t *u_svc = CAST_SVC(unit);
	assert(u_svc);

	ini_section_t *section = ini_get_section(ini_conf, section_name);
	if (section == NULL) {
		sysman_log(LVL_ERROR,
		    "Expected section '%s' in configuration of unit '%s'",
		    section_name, unit_name(unit));
		return ENOENT;
	}

	return config_load_ini_section(unit_configuration, section, u_svc,
	    text_parse);
}

static int unit_svc_start(unit_t *unit)
{
	unit_svc_t *u_svc = CAST_SVC(unit);
	assert(u_svc);

	
	// TODO think about unit's lifecycle (is STOPPED only acceptable?)
	assert(unit->state == STATE_STOPPED);

	int rc = task_spawnv(NULL, NULL, u_svc->exec_start.path,
	    u_svc->exec_start.argv);
	sysman_log(LVL_DEBUG2, "task_spawn(%s, %s)", u_svc->exec_start.path, u_svc->exec_start.argv[0]);
	if (rc != EOK) {
		unit->state = STATE_FAILED;
		return rc;
	}

	/*
	 * This is temporary workaround, until proper reporting from brokers
	 * about exposees will work. We assume the service succesfully starts
	 * in a moment.
	 */
	unit->state = STATE_STARTING;
	async_usleep(20000);
	unit->state = STATE_STARTED;

	/*
	 * Workaround to see log output even after devman starts (and overrides
	 * kernel's frame buffer. It's here since devman is started as a
	 * service (however not all services are devman...).
	 */
	if (console_kcon()) {
		sysman_log(LVL_DEBUG2, "%s: Kconsole grabbed.", __func__);
	} else {
		sysman_log(LVL_DEBUG2, "%s: no kconsole.", __func__);
	}

	return EOK;
}

static void unit_svc_exposee_created(unit_t *unit)
{
	// TODO implement
}

static void unit_svc_fail(unit_t *unit)
{
	// TODO implement
}

DEFINE_UNIT_VMT(unit_svc)

