/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	ui_pbutton_clicked(dialog->bok);

	/* Button callback with callback not implemented */
	ui_msg_dialog_set_cb(dialog, &dummy_msg_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->bok);

	/* Button callback with real callback set */
	resp.button = false;
	resp.bnum = 123;
	ui_msg_dialog_set_cb(dialog, &test_msg_dialog_cb, &resp);
	ui_pbutton_clicked(dialog->bok);
	PCUT_ASSERT_TRUE(resp.button);
	PCUT_ASSERT_INT_EQUALS(0, resp.bnum);

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
