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

/** @addtogroup libui
 * @{
 */
/**
 * @file Test control
 *
 * Test control allows to read the arguments of and inject the responses
 * to all control methods.
 */

#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/testctl.h>
#include "../private/testctl.h"

static void test_ctl_destroy(void *);
static errno_t test_ctl_paint(void *);
static ui_evclaim_t test_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);
static void test_ctl_unfocus(void *, unsigned);

ui_control_ops_t ui_test_ctl_ops = {
	.destroy = test_ctl_destroy,
	.paint = test_ctl_paint,
	.kbd_event = test_ctl_kbd_event,
	.pos_event = test_ctl_pos_event,
	.unfocus = test_ctl_unfocus
};

static void test_ctl_destroy(void *arg)
{
	ui_test_ctl_t *testctl = (ui_test_ctl_t *)arg;
	ui_tc_resp_t *resp = testctl->resp;

	resp->destroy = true;
	ui_test_ctl_destroy(testctl);
}

static errno_t test_ctl_paint(void *arg)
{
	ui_test_ctl_t *testctl = (ui_test_ctl_t *)arg;
	ui_tc_resp_t *resp = testctl->resp;

	resp->paint = true;
	return resp->rc;
}

static ui_evclaim_t test_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_test_ctl_t *testctl = (ui_test_ctl_t *)arg;
	ui_tc_resp_t *resp = testctl->resp;

	resp->kbd = true;
	resp->kevent = *event;

	return resp->claim;
}

static ui_evclaim_t test_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_test_ctl_t *testctl = (ui_test_ctl_t *)arg;
	ui_tc_resp_t *resp = testctl->resp;

	resp->pos = true;
	resp->pevent = *event;

	return resp->claim;
}

static void test_ctl_unfocus(void *arg, unsigned nfocus)
{
	ui_test_ctl_t *testctl = (ui_test_ctl_t *)arg;
	ui_tc_resp_t *resp = testctl->resp;

	resp->unfocus = true;
	resp->unfocus_nfocus = nfocus;
}

/** Create new test control.
 *
 * @param resp Response structure
 * @param rtest Place to store pointer to new test control
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_test_ctl_create(ui_tc_resp_t *resp, ui_test_ctl_t **rtest)
{
	ui_test_ctl_t *test;
	errno_t rc;

	test = calloc(1, sizeof(ui_test_ctl_t));
	if (test == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_test_ctl_ops, (void *)test, &test->control);
	if (rc != EOK) {
		free(test);
		return rc;
	}

	test->resp = resp;
	*rtest = test;
	return EOK;
}

/** Destroy test control.
 *
 * @param test Test control or @c NULL
 */
void ui_test_ctl_destroy(ui_test_ctl_t *test)
{
	if (test == NULL)
		return;

	ui_control_delete(test->control);
	free(test);
}

/** Get base control from test control.
 *
 * @param test Test control
 * @return Control
 */
ui_control_t *ui_test_ctl_ctl(ui_test_ctl_t *test)
{
	return test->control;
}

/** @}
 */
