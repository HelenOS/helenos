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

/** ui_suspend() / ui_resume() do nothing if we don't have a console,
 * ui_is_suspended() returns suspend status
 */
PCUT_TEST(suspend_resume)
{
	ui_t *ui = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	PCUT_ASSERT_FALSE(ui_is_suspended(ui));

	rc = ui_suspend(ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(ui_is_suspended(ui));

	rc = ui_resume(ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(ui_is_suspended(ui));

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

/** ui_is_get_rect() */
PCUT_TEST(get_rect)
{
	ui_t *ui = NULL;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ui);

	/* This won't work without a display service */
	rc = ui_get_rect(ui, &rect);
	PCUT_ASSERT_ERRNO_VAL(ENOTSUP, rc);

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
