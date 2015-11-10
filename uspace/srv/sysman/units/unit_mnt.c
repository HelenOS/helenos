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
#include <stdlib.h>
#include <vfs/vfs.h>
#include <str.h>

#include "log.h"
#include "sysman.h"
#include "unit.h"

static const char *section_name = "Mount";

static config_item_t unit_configuration[] = {
	{"What",      &config_parse_string, offsetof(unit_mnt_t, device),     NULL},
	{"Where",     &config_parse_string, offsetof(unit_mnt_t, mountpoint), NULL},
	{"Type",      &config_parse_string, offsetof(unit_mnt_t, type),       NULL},
	{"Autostart", &config_parse_bool,   offsetof(unit_mnt_t, autostart),  "true"},
	{"Blocking",  &config_parse_bool,   offsetof(unit_mnt_t, blocking),   "true"},
	CONFIGURATION_ITEM_SENTINEL
};

typedef struct {
	char *type;
	char *mountpoint;
	char *device;
	char *options;
	unsigned int flags;
	unsigned int instance;

	unit_t *unit;
	bool owner;
} mount_data_t;

static void mount_data_destroy(mount_data_t **mnt_data_ptr)
{
	assert(mnt_data_ptr);
	if (*mnt_data_ptr == NULL) {
		return;
	}

	mount_data_t *mnt_data = *mnt_data_ptr;
	free(mnt_data->type);
	free(mnt_data->mountpoint);
	free(mnt_data->device);
	free(mnt_data->options);

	free(mnt_data);
	*mnt_data_ptr = NULL;
}

static bool mount_data_copy(mount_data_t *src, mount_data_t **dst_ptr)
{
	mount_data_t *dst = malloc(sizeof(mount_data_t));
	if (dst == NULL) {
		goto fail;
	}

	dst->type = str_dup(src->type);
	if (dst->type == NULL)
		goto fail;

	dst->mountpoint = str_dup(src->mountpoint);
	if (dst->mountpoint == NULL)
		goto fail;

	dst->device = str_dup(src->device);
	if (dst->device == NULL)
		goto fail;

	dst->options = src->options ? str_dup(src->options) : NULL;
	if (src->options != NULL && dst->options == NULL)
		goto fail;

	dst->flags = src->flags;
	dst->instance = src->instance;
	dst->unit = src->unit;
	dst->owner = true;

	*dst_ptr = dst;
	return true;

fail:
	mount_data_destroy(&dst);
	return false;
}

static void unit_mnt_init(unit_t *unit)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);
}

static void unit_mnt_destroy(unit_t *unit)
{
	assert(unit->type == UNIT_MOUNT);
	unit_mnt_t *u_mnt = CAST_MNT(unit);

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

static int mount_exec(void *arg)
{
	mount_data_t *mnt_data = arg;
	sysman_log(LVL_DEBUG2, "%s(%p, %p, %p, %p, %x, %u)",
	    __func__,
	    mnt_data->type, mnt_data->mountpoint, mnt_data->device, mnt_data->options,
	    mnt_data->flags, mnt_data->instance);

	int rc = mount(mnt_data->type, mnt_data->mountpoint, mnt_data->device,
	    mnt_data->options ? mnt_data->options : "",
	    mnt_data->flags, mnt_data->instance);

	if (rc == EOK) {
		sysman_log(LVL_DEBUG, "Mount ('%s') mounted",
		    unit_name(mnt_data->unit));
		/*
		 * Emulate future VFS broker fibril that notifies about created
		 * exposee.
		 * Difference: It'll notify exposee name only, we'll have to
		 * match it...
		 */
		sysman_raise_event(&sysman_event_unit_exposee_created,
		    mnt_data->unit);
	} else {
		sysman_log(LVL_ERROR, "Mount ('%s') failed (%i)",
		    unit_name(mnt_data->unit), rc);
		/*
		 * Think about analogy of this event, probably timeout or sthing
		 */
		sysman_raise_event(&sysman_event_unit_failed,
		    mnt_data->unit);
	}

	if (mnt_data->owner) {
		mount_data_destroy(&mnt_data);
	}

	return EOK;
}

static int unit_mnt_start(unit_t *unit)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);
	/* autostart implies blocking */
	assert(!u_mnt->autostart || u_mnt->blocking);

	
	// TODO think about unit's lifecycle (is STOPPED only acceptable?)
	assert(unit->state == STATE_STOPPED);

	mount_data_t mnt_data;
	memset(&mnt_data, 0, sizeof(mnt_data));
	mnt_data.type       = u_mnt->type;
	mnt_data.mountpoint = u_mnt->mountpoint;
	mnt_data.device     = u_mnt->device;
	/* TODO use other mount parameters
	 * mnt_data.options    = u_mnt->options;
	 * mnt_data.instance   = u_mnt->instance;
	 */

	mnt_data.flags |= u_mnt->blocking ? IPC_FLAG_BLOCKING : 0;
	mnt_data.flags |= u_mnt->autostart ? IPC_AUTOSTART : 0;
	mnt_data.unit = unit;

	if (u_mnt->blocking) {
		mount_data_t *heap_mnt_data = NULL;
		if (!mount_data_copy(&mnt_data, &heap_mnt_data)) {
			return ENOMEM;
		}
		fid_t fib = fibril_create(&mount_exec, heap_mnt_data);
		unit->state = STATE_STARTING;
		fibril_add_ready(fib);
	} else {
		unit->state = STATE_STARTING;
		mount_exec(&mnt_data);
	}

	return EOK;
}

static int unit_mnt_stop(unit_t *unit)
{
	unit_mnt_t *u_mnt = CAST_MNT(unit);
	assert(u_mnt);
	/* autostart implies blocking */
	assert(!u_mnt->autostart || u_mnt->blocking);

	
	// TODO think about unit's lifecycle (is STOPPED only acceptable?)
	// note: we should never hit STATE_STARTING, since it'd mean there are
	//       two jobs running at once (unless job cancellation is implemented)
	assert(unit->state == STATE_STARTED);

	/*
	 * We don't expect unmount to be blocking, since if some files are
	 * being used, it'd return EBUSY immediately. That's why we call
	 * unmount synchronously in the event loop fibril.
	 */
	int rc = unmount(u_mnt->mountpoint);

	if (rc == EOK) {
		unit->state = STATE_STOPPED;
		return EOK;
	} else if (rc == EBUSY) {
		assert(unit->state == STATE_STARTED);
		return EBUSY;
	} else {
		/*
		 * Mount may be still usable, but be conservative and mark unit
		 * as failed.
		 */
		unit->state = STATE_FAILED;
		return rc;
	}
}

static void unit_mnt_exposee_created(unit_t *unit)
{
	assert(CAST_MNT(unit));
	assert(unit->state == STATE_STOPPED || unit->state == STATE_STARTING);

	if (str_cmp(unit_name(unit), "rootfs.mnt") == 0) {
		sysman_log_tofile();
	}
	unit->state = STATE_STARTED;
	unit_notify_state(unit);
}

static void unit_mnt_fail(unit_t *unit)
{
	assert(CAST_MNT(unit));
	assert(unit->state == STATE_STARTING);

	unit->state = STATE_FAILED;
	unit_notify_state(unit);
}


DEFINE_UNIT_VMT(unit_mnt)

