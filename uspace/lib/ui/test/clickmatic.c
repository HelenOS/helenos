/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <pcut/pcut.h>
#include <ui/clickmatic.h>
#include <ui/ui.h>
#include "../private/clickmatic.h"

PCUT_INIT;

PCUT_TEST_SUITE(clickmatic);

static void test_clicked(ui_clickmatic_t *, void *);

static ui_clickmatic_cb_t test_cb = {
	.clicked = test_clicked
};

typedef struct {
	int clicked_cnt;
} test_resp_t;

/** Create and destroy clickmatic */
PCUT_TEST(create_destroy)
{
	ui_t *ui = NULL;
	ui_clickmatic_t *clickmatic = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_clickmatic_create(ui, &clickmatic);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(clickmatic);

	ui_clickmatic_destroy(clickmatic);
	ui_destroy(ui);
}

/** ui_clickmatic_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_clickmatic_destroy(NULL);
}

/** ui_clickmatic_set_cb() sets internal fields */
PCUT_TEST(set_cb)
{
	ui_t *ui = NULL;
	ui_clickmatic_t *clickmatic = NULL;
	test_resp_t test_resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_clickmatic_create(ui, &clickmatic);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(clickmatic);

	ui_clickmatic_set_cb(clickmatic, &test_cb, (void *)&test_resp);
	PCUT_ASSERT_EQUALS(&test_cb, clickmatic->cb);
	PCUT_ASSERT_EQUALS((void *)&test_resp, clickmatic->arg);

	ui_clickmatic_destroy(clickmatic);
	ui_destroy(ui);
}

/** Pressing and releasing clickmatic generates one event */
PCUT_TEST(press_release)
{
	ui_t *ui = NULL;
	ui_clickmatic_t *clickmatic = NULL;
	test_resp_t test_resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_clickmatic_create(ui, &clickmatic);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(clickmatic);

	test_resp.clicked_cnt = 0;

	ui_clickmatic_set_cb(clickmatic, &test_cb, (void *)&test_resp);
	PCUT_ASSERT_EQUALS(&test_cb, clickmatic->cb);
	PCUT_ASSERT_EQUALS((void *)&test_resp, clickmatic->arg);

	PCUT_ASSERT_INT_EQUALS(0, test_resp.clicked_cnt);

	ui_clickmatic_press(clickmatic);
	ui_clickmatic_release(clickmatic);

	PCUT_ASSERT_INT_EQUALS(1, test_resp.clicked_cnt);

	ui_clickmatic_destroy(clickmatic);
	ui_destroy(ui);
}

static void test_clicked(ui_clickmatic_t *clickmatic, void *arg)
{
	test_resp_t *test_resp = (test_resp_t *)arg;

	++test_resp->clicked_cnt;
}

PCUT_EXPORT(clickmatic);
