/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * DATA, OR PROFITS; OR BUSINESS INTvvhhzccgggrERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <mem.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <pcut/pcut.h>
#include <ui/control.h>
#include <stdbool.h>
#include <types/ui/event.h>

PCUT_INIT;

PCUT_TEST_SUITE(control);

static void test_ctl_destroy(void *);
static errno_t test_ctl_paint(void *);
static ui_evclaim_t test_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);
static void test_ctl_unfocus(void *);

static ui_control_ops_t test_ctl_ops = {
	.destroy = test_ctl_destroy,
	.paint = test_ctl_paint,
	.kbd_event = test_ctl_kbd_event,
	.pos_event = test_ctl_pos_event,
	.unfocus = test_ctl_unfocus
};

/** Test response */
typedef struct {
	/** Claim to return */
	ui_evclaim_t claim;
	/** Result code to return */
	errno_t rc;

	/** @c true iff destroy was called */
	bool destroy;

	/** @c true iff paint was called */
	bool paint;

	/** @c true iff kbd_event was called */
	bool kbd;
	/** Keyboard event that was sent */
	kbd_event_t kevent;

	/** @c true iff pos_event was called */
	bool pos;
	/** Position event that was sent */
	pos_event_t pevent;

	/** @c true iff unfocus was called */
	bool unfocus;
} test_resp_t;

/** Allocate and deallocate control */
PCUT_TEST(new_delete)
{
	ui_control_t *control = NULL;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, NULL, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_delete(control);
}

/** ui_control_delete() can take NULL argument (no-op) */
PCUT_TEST(delete_null)
{
	ui_control_delete(NULL);
}

/** Test sending destroy request to control */
PCUT_TEST(destroy)
{
	ui_control_t *control = NULL;
	test_resp_t resp;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	resp.rc = EOK;
	resp.destroy = false;

	ui_control_destroy(control);
	PCUT_ASSERT_TRUE(resp.destroy);
}

/** Test sending paint request to control */
PCUT_TEST(paint)
{
	ui_control_t *control = NULL;
	test_resp_t resp;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	resp.rc = EOK;
	resp.paint = false;

	rc = ui_control_paint(control);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	resp.rc = EINVAL;
	resp.paint = false;

	rc = ui_control_paint(control);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_control_delete(control);
}

/** Test sending keyboard event to control */
PCUT_TEST(kbd_event)
{
	ui_control_t *control = NULL;
	test_resp_t resp;
	kbd_event_t event;
	ui_evclaim_t claim;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	resp.claim = ui_claimed;
	resp.kbd = false;
	event.type = KEY_PRESS;
	event.key = KC_2;
	event.mods = KM_LSHIFT;
	event.c = '@';

	claim = ui_control_kbd_event(control, &event);
	PCUT_ASSERT_EQUALS(resp.claim, claim);
	PCUT_ASSERT_TRUE(resp.kbd);
	PCUT_ASSERT_EQUALS(resp.kevent.type, event.type);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.key, event.key);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.mods, event.mods);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.c, event.c);

	ui_control_delete(control);
}

/** Test sending position event to control */
PCUT_TEST(pos_event)
{
	ui_control_t *control = NULL;
	test_resp_t resp;
	pos_event_t event;
	ui_evclaim_t claim;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	resp.claim = ui_claimed;
	resp.pos = false;
	event.pos_id = 1;
	event.type = POS_PRESS;
	event.btn_num = 2;
	event.hpos = 3;
	event.vpos = 4;

	claim = ui_control_pos_event(control, &event);
	PCUT_ASSERT_EQUALS(resp.claim, claim);
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.pos_id, event.pos_id);
	PCUT_ASSERT_EQUALS(resp.pevent.type, event.type);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.btn_num, event.btn_num);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.hpos, event.hpos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.vpos, event.vpos);

	ui_control_delete(control);
}

/** Test sending unfocus to control */
PCUT_TEST(unfocus)
{
	ui_control_t *control = NULL;
	test_resp_t resp;
	errno_t rc;

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(control);

	resp.unfocus = false;

	ui_control_unfocus(control);
	PCUT_ASSERT_TRUE(resp.unfocus);

	ui_control_delete(control);
}

static void test_ctl_destroy(void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->destroy = true;
}

static errno_t test_ctl_paint(void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->paint = true;
	return resp->rc;
}

static ui_evclaim_t test_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->kbd = true;
	resp->kevent = *event;

	return resp->claim;
}

static ui_evclaim_t test_ctl_pos_event(void *arg, pos_event_t *event)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->pos = true;
	resp->pevent = *event;

	return resp->claim;
}

static void test_ctl_unfocus(void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->unfocus = true;
}

PCUT_EXPORT(control);
