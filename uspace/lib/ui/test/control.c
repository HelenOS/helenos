/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <ui/testctl.h>
#include <stdbool.h>
#include <types/ui/event.h>
#include "../private/testctl.h"

PCUT_INIT;

PCUT_TEST_SUITE(control);

/** Allocate and deallocate control */
PCUT_TEST(new_delete)
{
	ui_control_t *control = NULL;
	errno_t rc;

	rc = ui_control_new(&ui_test_ctl_ops, NULL, &control);
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
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	resp.rc = EOK;
	resp.destroy = false;

	ui_control_destroy(ui_test_ctl_ctl(testctl));
	PCUT_ASSERT_TRUE(resp.destroy);
}

/** Test sending paint request to control */
PCUT_TEST(paint)
{
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	resp.rc = EOK;
	resp.paint = false;

	rc = ui_control_paint(ui_test_ctl_ctl(testctl));
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	resp.rc = EINVAL;
	resp.paint = false;

	rc = ui_control_paint(ui_test_ctl_ctl(testctl));
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_test_ctl_destroy(testctl);
}

/** Test sending keyboard event to control */
PCUT_TEST(kbd_event)
{
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	kbd_event_t event;
	ui_evclaim_t claim;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	resp.claim = ui_claimed;
	resp.kbd = false;
	event.type = KEY_PRESS;
	event.key = KC_2;
	event.mods = KM_LSHIFT;
	event.c = '@';

	claim = ui_control_kbd_event(ui_test_ctl_ctl(testctl), &event);
	PCUT_ASSERT_EQUALS(resp.claim, claim);
	PCUT_ASSERT_TRUE(resp.kbd);
	PCUT_ASSERT_EQUALS(resp.kevent.type, event.type);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.key, event.key);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.mods, event.mods);
	PCUT_ASSERT_INT_EQUALS(resp.kevent.c, event.c);

	ui_test_ctl_destroy(testctl);
}

/** Test sending position event to control */
PCUT_TEST(pos_event)
{
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	pos_event_t event;
	ui_evclaim_t claim;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	resp.claim = ui_claimed;
	resp.pos = false;
	event.pos_id = 1;
	event.type = POS_PRESS;
	event.btn_num = 2;
	event.hpos = 3;
	event.vpos = 4;

	claim = ui_control_pos_event(ui_test_ctl_ctl(testctl), &event);
	PCUT_ASSERT_EQUALS(resp.claim, claim);
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.pos_id, event.pos_id);
	PCUT_ASSERT_EQUALS(resp.pevent.type, event.type);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.btn_num, event.btn_num);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.hpos, event.hpos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.vpos, event.vpos);

	ui_test_ctl_destroy(testctl);
}

/** Test sending unfocus to control */
PCUT_TEST(unfocus)
{
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	resp.unfocus = false;

	ui_control_unfocus(ui_test_ctl_ctl(testctl), 42);
	PCUT_ASSERT_TRUE(resp.unfocus);
	PCUT_ASSERT_INT_EQUALS(42, resp.unfocus_nfocus);

	ui_test_ctl_destroy(testctl);
}

PCUT_EXPORT(control);
