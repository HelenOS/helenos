/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/promptdialog.h>
#include "../private/promptdialog.h"
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(prompt_dialog);

static void test_dialog_bok(ui_prompt_dialog_t *, void *, const char *);
static void test_dialog_bcancel(ui_prompt_dialog_t *, void *);
static void test_dialog_close(ui_prompt_dialog_t *, void *);

static ui_prompt_dialog_cb_t test_prompt_dialog_cb = {
	.bok = test_dialog_bok,
	.bcancel = test_dialog_bcancel,
	.close = test_dialog_close
};

static ui_prompt_dialog_cb_t dummy_prompt_dialog_cb = {
};

typedef struct {
	bool bok;
	const char *fname;
	bool bcancel;
	bool close;
} test_cb_resp_t;

/** Create and destroy prompt dialog */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_prompt_dialog_params_t params;
	ui_prompt_dialog_t *dialog = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_prompt_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_prompt_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	ui_prompt_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_prompt_dialog_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_prompt_dialog_destroy(NULL);
}

/** Button click invokes callback set via ui_prompt_dialog_set_cb() */
PCUT_TEST(button_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_prompt_dialog_params_t params;
	ui_prompt_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_prompt_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_prompt_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* OK button callback with no callbacks set */
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with callback not implemented */
	ui_prompt_dialog_set_cb(dialog, &dummy_prompt_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with real callback set */
	resp.bok = false;
	ui_prompt_dialog_set_cb(dialog, &test_prompt_dialog_cb, &resp);
	ui_pbutton_clicked(dialog->bok);
	PCUT_ASSERT_TRUE(resp.bok);

	ui_prompt_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Sending window close request invokes callback set via
 * ui_prompt_dialog_set_cb()
 */
PCUT_TEST(close_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_prompt_dialog_params_t params;
	ui_prompt_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_prompt_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_prompt_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Button callback with no callbacks set */
	ui_window_send_close(dialog->window);

	/* Button callback with unfocus callback not implemented */
	ui_prompt_dialog_set_cb(dialog, &dummy_prompt_dialog_cb, NULL);
	ui_window_send_close(dialog->window);

	/* Button callback with real callback set */
	resp.close = false;
	ui_prompt_dialog_set_cb(dialog, &test_prompt_dialog_cb, &resp);
	ui_window_send_close(dialog->window);
	PCUT_ASSERT_TRUE(resp.close);

	ui_prompt_dialog_destroy(dialog);
	ui_destroy(ui);
}

static void test_dialog_bok(ui_prompt_dialog_t *dialog, void *arg,
    const char *fname)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bok = true;
}

static void test_dialog_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bcancel = true;
}

static void test_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

PCUT_EXPORT(prompt_dialog);
