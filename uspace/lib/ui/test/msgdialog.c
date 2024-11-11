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

#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/msgdialog.h>
#include "../private/msgdialog.h"
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(msg_dialog);

static void test_dialog_button(ui_msg_dialog_t *, void *, unsigned);
static void test_dialog_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t test_msg_dialog_cb = {
	.button = test_dialog_button,
	.close = test_dialog_close
};

static ui_msg_dialog_cb_t dummy_msg_dialog_cb = {
};

typedef struct {
	bool button;
	unsigned bnum;
	bool close;
} test_cb_resp_t;

/** Create and destroy message dialog */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_msg_dialog_params_init(&params);
	params.caption = "Message";
	params.text = "Hello";

	rc = ui_msg_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	ui_msg_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_msg_dialog_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_msg_dialog_destroy(NULL);
}

/** Button click invokes callback set via ui_msg_dialog_set_cb() */
PCUT_TEST(button_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog = NULL;
	unsigned i;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_msg_dialog_params_init(&params);
	params.caption = "Message";
	params.text = "Hello";
	params.choice = umdc_ok_cancel;

	rc = ui_msg_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Button callback with no callbacks set */
	ui_pbutton_clicked(dialog->btn[0]);

	/* Button callback with callback not implemented */
	ui_msg_dialog_set_cb(dialog, &dummy_msg_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->btn[0]);

	for (i = 0; i < 2; i++) {
		/* Button callback with real callback set */
		resp.button = false;
		resp.bnum = 123;
		ui_msg_dialog_set_cb(dialog, &test_msg_dialog_cb, &resp);
		ui_pbutton_clicked(dialog->btn[i]);
		PCUT_ASSERT_TRUE(resp.button);
		PCUT_ASSERT_INT_EQUALS(i, resp.bnum);
	}

	ui_msg_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Sending window close request invokes callback set via
 * ui_msg_dialog_set_cb()
 */
PCUT_TEST(close_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_msg_dialog_params_init(&params);
	params.caption = "Message";
	params.text = "Hello";

	rc = ui_msg_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Button callback with no callbacks set */
	ui_window_send_close(dialog->window);

	/* Button callback with unfocus callback not implemented */
	ui_msg_dialog_set_cb(dialog, &dummy_msg_dialog_cb, NULL);
	ui_window_send_close(dialog->window);

	/* Button callback with real callback set */
	resp.close = false;
	ui_msg_dialog_set_cb(dialog, &test_msg_dialog_cb, &resp);
	ui_window_send_close(dialog->window);
	PCUT_ASSERT_TRUE(resp.close);

	ui_msg_dialog_destroy(dialog);
	ui_destroy(ui);
}

static void test_dialog_button(ui_msg_dialog_t *dialog, void *arg,
    unsigned bnum)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->button = true;
	resp->bnum = bnum;
}

static void test_dialog_close(ui_msg_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

PCUT_EXPORT(msg_dialog);
