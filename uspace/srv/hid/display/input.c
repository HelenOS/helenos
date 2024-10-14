/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

/** @addtogroup display
 * @{
 */
/** @file Input events
 */

#include <errno.h>
#include <inttypes.h>
#include <io/input.h>
#include <io/log.h>
#include <loc.h>
#include <str_error.h>
#include "display.h"
#include "ievent.h"
#include "input.h"
#include "main.h"

static errno_t ds_input_ev_active(input_t *);
static errno_t ds_input_ev_deactive(input_t *);
static errno_t ds_input_ev_key(input_t *, unsigned, kbd_event_type_t, keycode_t,
    keymod_t, char32_t);
static errno_t ds_input_ev_move(input_t *, unsigned, int, int);
static errno_t ds_input_ev_abs_move(input_t *, unsigned, unsigned, unsigned,
    unsigned, unsigned);
static errno_t ds_input_ev_button(input_t *, unsigned, int, int);
static errno_t ds_input_ev_dclick(input_t *, unsigned, int);

static input_ev_ops_t ds_input_ev_ops = {
	.active = ds_input_ev_active,
	.deactive = ds_input_ev_deactive,
	.key = ds_input_ev_key,
	.move = ds_input_ev_move,
	.abs_move = ds_input_ev_abs_move,
	.button = ds_input_ev_button,
	.dclick = ds_input_ev_dclick
};

static errno_t ds_input_ev_active(input_t *input)
{
	return EOK;
}

static errno_t ds_input_ev_deactive(input_t *input)
{
	return EOK;
}

static errno_t ds_input_ev_key(input_t *input, unsigned kbd_id,
    kbd_event_type_t type, keycode_t key, keymod_t mods, char32_t c)
{
	ds_display_t *disp = (ds_display_t *) input->user;
	kbd_event_t event;
	errno_t rc;

	event.kbd_id = kbd_id;
	event.type = type;
	event.key = key;
	event.mods = mods;
	event.c = c;

	ds_display_lock(disp);
	rc = ds_ievent_post_kbd(disp, &event);
	ds_display_unlock(disp);
	return rc;
}

static errno_t ds_input_ev_move(input_t *input, unsigned pos_id, int dx, int dy)
{
	ds_display_t *disp = (ds_display_t *) input->user;
	ptd_event_t event;
	errno_t rc;

	event.pos_id = pos_id;
	event.type = PTD_MOVE;
	event.dmove.x = dx;
	event.dmove.y = dy;

	ds_display_lock(disp);
	rc = ds_ievent_post_ptd(disp, &event);
	ds_display_unlock(disp);
	return rc;
}

static errno_t ds_input_ev_abs_move(input_t *input, unsigned pos_id, unsigned x,
    unsigned y, unsigned max_x, unsigned max_y)
{
	ds_display_t *disp = (ds_display_t *) input->user;
	ptd_event_t event;
	errno_t rc;

	event.pos_id = pos_id;
	event.type = PTD_ABS_MOVE;
	event.apos.x = x;
	event.apos.y = y;
	event.abounds.p0.x = 0;
	event.abounds.p0.y = 0;
	event.abounds.p1.x = max_x + 1;
	event.abounds.p1.y = max_y + 1;

	ds_display_lock(disp);
	rc = ds_ievent_post_ptd(disp, &event);
	ds_display_unlock(disp);
	return rc;
}

static errno_t ds_input_ev_button(input_t *input, unsigned pos_id, int bnum,
    int bpress)
{
	ds_display_t *disp = (ds_display_t *) input->user;
	ptd_event_t event;
	errno_t rc;

	event.pos_id = pos_id;
	event.type = bpress ? PTD_PRESS : PTD_RELEASE;
	event.btn_num = bnum;
	event.dmove.x = 0;
	event.dmove.y = 0;

	ds_display_lock(disp);
	rc = ds_ievent_post_ptd(disp, &event);
	ds_display_unlock(disp);
	return rc;
}

static errno_t ds_input_ev_dclick(input_t *input, unsigned pos_id, int bnum)
{
	ds_display_t *disp = (ds_display_t *) input->user;
	ptd_event_t event;
	errno_t rc;

	event.pos_id = pos_id;
	event.type = PTD_DCLICK;
	event.btn_num = bnum;
	event.dmove.x = 0;
	event.dmove.y = 0;

	ds_display_lock(disp);
	rc = ds_ievent_post_ptd(disp, &event);
	ds_display_unlock(disp);
	return rc;
}

/** Open input service.
 *
 * @param display Display
 * @return EOK on success or an error code
 */
errno_t ds_input_open(ds_display_t *display)
{
	async_sess_t *sess;
	service_id_t dsid;
	const char *svc = "hid/input";

	errno_t rc = loc_service_get_id(svc, &dsid, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Input service %s not found",
		    svc);
		return rc;
	}

	sess = loc_service_connect(dsid, INTERFACE_INPUT, 0);
	if (sess == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Unable to connect to input service %s", svc);
		return EIO;
	}

	rc = input_open(sess, &ds_input_ev_ops, (void *) display,
	    &display->input);
	if (rc != EOK) {
		async_hangup(sess);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Unable to communicate with service %s (%s)",
		    svc, str_error(rc));
		return rc;
	}

	input_activate(display->input);
	return EOK;
}

void ds_input_close(ds_display_t *display)
{
	input_close(display->input);
	display->input = NULL;
}

/** @}
 */
