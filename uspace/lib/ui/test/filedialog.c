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
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/filedialog.h>
#include "../private/filedialog.h"
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(file_dialog);

static void test_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void test_dialog_bcancel(ui_file_dialog_t *, void *);
static void test_dialog_close(ui_file_dialog_t *, void *);

static ui_file_dialog_cb_t test_file_dialog_cb = {
	.bok = test_dialog_bok,
	.bcancel = test_dialog_bcancel,
	.close = test_dialog_close
};

static ui_file_dialog_cb_t dummy_file_dialog_cb = {
};

typedef struct {
	bool bok;
	const char *fname;
	bool bcancel;
	bool close;
} test_cb_resp_t;

/** Create and destroy file dialog */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_file_dialog_params_t params;
	ui_file_dialog_t *dialog = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_file_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	ui_file_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_file_dialog_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_file_dialog_destroy(NULL);
}

/** Button click invokes callback set via ui_file_dialog_set_cb() */
PCUT_TEST(button_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_file_dialog_params_t params;
	ui_file_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_file_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* OK button callback with no callbacks set */
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with callback not implemented */
	ui_file_dialog_set_cb(dialog, &dummy_file_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with real callback set */
	resp.bok = false;
	ui_file_dialog_set_cb(dialog, &test_file_dialog_cb, &resp);
	ui_pbutton_clicked(dialog->bok);
	PCUT_ASSERT_TRUE(resp.bok);

	ui_file_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Sending window close request invokes callback set via
 * ui_file_dialog_set_cb()
 */
PCUT_TEST(close_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_file_dialog_params_t params;
	ui_file_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_dialog_params_init(&params);
	params.caption = "Open";

	rc = ui_file_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Button callback with no callbacks set */
	ui_window_send_close(dialog->window);

	/* Button callback with unfocus callback not implemented */
	ui_file_dialog_set_cb(dialog, &dummy_file_dialog_cb, NULL);
	ui_window_send_close(dialog->window);

	/* Button callback with real callback set */
	resp.close = false;
	ui_file_dialog_set_cb(dialog, &test_file_dialog_cb, &resp);
	ui_window_send_close(dialog->window);
	PCUT_ASSERT_TRUE(resp.close);

	ui_file_dialog_destroy(dialog);
	ui_destroy(ui);
}

static void test_dialog_bok(ui_file_dialog_t *dialog, void *arg,
    const char *fname)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bok = true;
}

static void test_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bcancel = true;
}

static void test_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

PCUT_EXPORT(file_dialog);
