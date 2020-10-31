/*
 * Copyright (c) 2020 Jiri Svoboda
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

#include <mem.h>
#include <io/pos_event.h>
#include <pcut/pcut.h>
#include <ui/control.h>
#include <stdbool.h>
#include <types/ui/event.h>

PCUT_INIT;

PCUT_TEST_SUITE(control);

static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);

static ui_control_ops_t test_ctl_ops = {
	.pos_event = test_ctl_pos_event
};

/** Test response */
typedef struct {
	/** Claim to return */
	ui_evclaim_t claim;

	/** @c true iff pos_event was called */
	bool pos;
	/** Position event that was sent */
	pos_event_t pevent;
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

static ui_evclaim_t test_ctl_pos_event(void *arg, pos_event_t *event)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->pos = true;
	resp->pevent = *event;

	return resp->claim;
}

PCUT_EXPORT(control);
