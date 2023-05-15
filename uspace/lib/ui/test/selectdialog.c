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

#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/list.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/selectdialog.h>
#include "../private/list.h"
#include "../private/selectdialog.h"
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(select_dialog);

static void test_dialog_bok(ui_select_dialog_t *, void *, void *);
static void test_dialog_bcancel(ui_select_dialog_t *, void *);
static void test_dialog_close(ui_select_dialog_t *, void *);

static ui_select_dialog_cb_t test_select_dialog_cb = {
	.bok = test_dialog_bok,
	.bcancel = test_dialog_bcancel,
	.close = test_dialog_close
};

static ui_select_dialog_cb_t dummy_select_dialog_cb = {
};

typedef struct {
	bool bok;
	const char *fname;
	bool bcancel;
	bool close;
} test_cb_resp_t;

/** Create and destroy select dialog */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_select_dialog_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_select_dialog_destroy(NULL);
}

/** Clicking OK invokes callback set via ui_select_dialog_set_cb() */
PCUT_TEST(bok_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* OK button callback with no callbacks set */
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with callback not implemented */
	ui_select_dialog_set_cb(dialog, &dummy_select_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->bok);

	/* OK button callback with real callback set */
	resp.bok = false;
	ui_select_dialog_set_cb(dialog, &test_select_dialog_cb, &resp);
	ui_pbutton_clicked(dialog->bok);
	PCUT_ASSERT_TRUE(resp.bok);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Clicking Cancel invokes callback set via ui_select_dialog_set_cb() */
PCUT_TEST(bcancel_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Cancel button callback with no callbacks set */
	ui_pbutton_clicked(dialog->bcancel);

	/* Cancel button callback with callback not implemented */
	ui_select_dialog_set_cb(dialog, &dummy_select_dialog_cb, NULL);
	ui_pbutton_clicked(dialog->bcancel);

	/* OK button callback with real callback set */
	resp.bcancel = false;
	ui_select_dialog_set_cb(dialog, &test_select_dialog_cb, &resp);
	ui_pbutton_clicked(dialog->bcancel);
	PCUT_ASSERT_TRUE(resp.bcancel);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Selecting a list entry invokes bok callback set via ui_select_dialog_set_cb() */
PCUT_TEST(lselect_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_list_entry_t *entry;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	ui_list_entry_attr_t attr;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Need an entry to select */
	ui_list_entry_attr_init(&attr);
	attr.caption = "Entry";
	rc = ui_select_dialog_append(dialog, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = ui_list_first(dialog->list);

	/* Select entry with no callbacks set */
	ui_list_selected(entry);

	/* Select entry with callback not implemented */
	ui_select_dialog_set_cb(dialog, &dummy_select_dialog_cb, NULL);
	ui_list_selected(entry);

	/* Select entry with real callback set */
	resp.bok = false;
	ui_select_dialog_set_cb(dialog, &test_select_dialog_cb, &resp);
	ui_list_selected(entry);
	PCUT_ASSERT_TRUE(resp.bok);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** Sending window close request invokes callback set via
 * ui_select_dialog_set_cb()
 */
PCUT_TEST(close_cb)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	/* Button callback with no callbacks set */
	ui_window_send_close(dialog->window);

	/* Button callback with unfocus callback not implemented */
	ui_select_dialog_set_cb(dialog, &dummy_select_dialog_cb, NULL);
	ui_window_send_close(dialog->window);

	/* Button callback with real callback set */
	resp.close = false;
	ui_select_dialog_set_cb(dialog, &test_select_dialog_cb, &resp);
	ui_window_send_close(dialog->window);
	PCUT_ASSERT_TRUE(resp.close);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_select_dialog_append() appends entries TBD */
PCUT_TEST(append)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	ui_list_entry_attr_t attr;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	PCUT_ASSERT_INT_EQUALS(0, dialog->list->entries_cnt);

	/* Add one entry */
	ui_list_entry_attr_init(&attr);
	attr.caption = "Entry";
	rc = ui_select_dialog_append(dialog, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, dialog->list->entries_cnt);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_select_dialog_paint() succeeds */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	rc = ui_select_dialog_paint(dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

/** ui_select_dialog_list() returns the UI list */
PCUT_TEST(list)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_select_dialog_params_t params;
	ui_select_dialog_t *dialog = NULL;
	ui_list_t *list;
	ui_list_entry_attr_t attr;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&params);
	params.caption = "Select one";
	params.prompt = "Please select";

	rc = ui_select_dialog_create(ui, &params, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dialog);

	list = ui_select_dialog_list(dialog);
	PCUT_ASSERT_NOT_NULL(list);

	PCUT_ASSERT_INT_EQUALS(0, ui_list_entries_cnt(list));

	/* Add one entry */
	ui_list_entry_attr_init(&attr);
	attr.caption = "Entry";
	rc = ui_select_dialog_append(dialog, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, ui_list_entries_cnt(list));

	ui_select_dialog_destroy(dialog);
	ui_destroy(ui);
}

static void test_dialog_bok(ui_select_dialog_t *dialog, void *arg,
    void *earg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bok = true;
}

static void test_dialog_bcancel(ui_select_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->bcancel = true;
}

static void test_dialog_close(ui_select_dialog_t *dialog, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

PCUT_EXPORT(select_dialog);
