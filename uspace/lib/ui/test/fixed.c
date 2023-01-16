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
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <pcut/pcut.h>
#include <stddef.h>
#include <ui/control.h>
#include <ui/fixed.h>
#include "../private/fixed.h"

PCUT_INIT;

PCUT_TEST_SUITE(fixed);

static void test_ctl_destroy(void *);
static errno_t test_ctl_paint(void *);
static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);
static void test_ctl_unfocus(void *, unsigned);

static ui_control_ops_t test_ctl_ops = {
	.destroy = test_ctl_destroy,
	.paint = test_ctl_paint,
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
	/** @c true iff pos_event was called */
	bool pos;
	/** Position event that was sent */
	pos_event_t pevent;
	/** @c true iff unfocus was called */
	bool unfocus;
	/** Number of remaining foci */
	unsigned unfocus_nfocus;
} test_resp_t;

/** Create and destroy button */
PCUT_TEST(create_destroy)
{
	ui_fixed_t *fixed = NULL;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(fixed);

	ui_fixed_destroy(fixed);
}

/** ui_fixed_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_fixed_destroy(NULL);
}

/** ui_fixed_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_fixed_t *fixed;
	ui_control_t *control;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_fixed_ctl(fixed);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** ui_fixed_add() / ui_fixed_remove() adds/removes control */
PCUT_TEST(add_remove)
{
	ui_fixed_t *fixed = NULL;
	ui_control_t *control;
	ui_fixed_elem_t *e;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = ui_fixed_first(fixed);
	PCUT_ASSERT_NULL(e);

	rc = ui_control_new(NULL, NULL, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_fixed_add(fixed, control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = ui_fixed_first(fixed);
	PCUT_ASSERT_NOT_NULL(e);
	PCUT_ASSERT_EQUALS(control, e->control);
	e = ui_fixed_next(e);
	PCUT_ASSERT_NULL(e);

	ui_fixed_remove(fixed, control);

	e = ui_fixed_first(fixed);
	PCUT_ASSERT_NULL(e);

	ui_fixed_destroy(fixed);
	ui_control_delete(control);
}

/** ui_fixed_destroy() delivers destroy request to control */
PCUT_TEST(destroy)
{
	ui_fixed_t *fixed = NULL;
	ui_control_t *control;
	test_resp_t resp;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_control_new(&test_ctl_ops, (void *) &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_fixed_add(fixed, control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.destroy = false;

	ui_fixed_destroy(fixed);
	PCUT_ASSERT_TRUE(resp.destroy);
}

/** ui_fixed_paint() delivers paint request to control */
PCUT_TEST(paint)
{
	ui_fixed_t *fixed = NULL;
	ui_control_t *control;
	test_resp_t resp;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_control_new(&test_ctl_ops, (void *) &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_fixed_add(fixed, control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.paint = false;
	resp.rc = EOK;

	rc = ui_fixed_paint(fixed);
	PCUT_ASSERT_EQUALS(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	resp.paint = false;
	resp.rc = EINVAL;

	rc = ui_fixed_paint(fixed);
	PCUT_ASSERT_EQUALS(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_fixed_destroy(fixed);
}

/** ui_fixed_pos_event() delivers position event to control */
PCUT_TEST(pos_event)
{
	ui_fixed_t *fixed = NULL;
	ui_control_t *control;
	ui_evclaim_t claim;
	pos_event_t event;
	test_resp_t resp;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_control_new(&test_ctl_ops, (void *) &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_fixed_add(fixed, control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.claim = ui_claimed;
	resp.pos = false;
	event.pos_id = 1;
	event.type = POS_PRESS;
	event.btn_num = 2;
	event.hpos = 3;
	event.vpos = 4;

	claim = ui_fixed_pos_event(fixed, &event);
	PCUT_ASSERT_EQUALS(resp.claim, claim);
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.pos_id, event.pos_id);
	PCUT_ASSERT_EQUALS(resp.pevent.type, event.type);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.btn_num, event.btn_num);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.hpos, event.hpos);
	PCUT_ASSERT_INT_EQUALS(resp.pevent.vpos, event.vpos);

	ui_fixed_destroy(fixed);
}

/** ui_fixed_unfocus() delivers unfocus notification to control */
PCUT_TEST(unfocus)
{
	ui_fixed_t *fixed = NULL;
	ui_control_t *control;
	test_resp_t resp;
	errno_t rc;

	rc = ui_fixed_create(&fixed);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_control_new(&test_ctl_ops, (void *) &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_fixed_add(fixed, control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.unfocus = false;

	ui_fixed_unfocus(fixed, 42);
	PCUT_ASSERT_TRUE(resp.unfocus);
	PCUT_ASSERT_INT_EQUALS(42, resp.unfocus_nfocus);

	ui_fixed_destroy(fixed);
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

static ui_evclaim_t test_ctl_pos_event(void *arg, pos_event_t *event)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->pos = true;
	resp->pevent = *event;

	return resp->claim;
}

static void test_ctl_unfocus(void *arg, unsigned nfocus)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->unfocus = true;
	resp->unfocus_nfocus = nfocus;
}

PCUT_EXPORT(fixed);
