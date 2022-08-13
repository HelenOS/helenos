/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <ui/ui.h>
#include "../private/ui.h"

PCUT_INIT;

PCUT_TEST_SUITE(ui);

/** Create and destroy UI with display */
PCUT_TEST(create_disp_destroy)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);
	PCUT_ASSERT_NULL(ui->display);

	ui_destroy(ui);
}

/** Create and destroy UI with console */
PCUT_TEST(create_cons_destroy)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_cons(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);
	PCUT_ASSERT_NULL(ui->console);

	ui_destroy(ui);
}

/** ui_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_destroy(NULL);
}

/** ui_suspend() / ui_resume() do nothing if we don't have a console */
PCUT_TEST(suspend_resume)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	rc = ui_suspend(ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = ui_resume(ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_destroy(ui);
}

/** ui_run() / ui_quit() */
PCUT_TEST(run_quit)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	/* Set exit flag */
	ui_quit(ui);

	/* ui_run() should return immediately */
	ui_run(ui);

	ui_destroy(ui);
}

/** ui_paint() */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_cons(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	/* In absence of windows ui_paint() should just return EOK */
	rc = ui_paint(ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_destroy(ui);
}

/** ui_is_textmode() */
PCUT_TEST(is_textmode)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp((display_t *)(-1), &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	PCUT_ASSERT_FALSE(ui_is_textmode(ui));

	ui_destroy(ui);

	rc = ui_create_cons((console_ctrl_t *)(-1), &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	PCUT_ASSERT_TRUE(ui_is_textmode(ui));

	ui_destroy(ui);
}

/** ui_is_fullscreen() */
PCUT_TEST(is_fullscreen)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp((display_t *)(-1), &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	PCUT_ASSERT_FALSE(ui_is_fullscreen(ui));

	ui_destroy(ui);

	rc = ui_create_cons((console_ctrl_t *)(-1), &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	PCUT_ASSERT_TRUE(ui_is_fullscreen(ui));

	ui_destroy(ui);
}

/** ui_lock(), ui_unlock() */
PCUT_TEST(lock_unlock)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp((display_t *)(-1), &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	ui_lock(ui);
	ui_unlock(ui);

	ui_destroy(ui);
}

PCUT_EXPORT(ui);
