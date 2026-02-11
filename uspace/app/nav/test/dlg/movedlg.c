/*
 * Copyright (c) 2026 Jiri Svoboda
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
#include <fmgt.h>
#include <pcut/pcut.h>
#include <ui/ui.h>
#include "../../dlg/movedlg.h"

PCUT_INIT;

PCUT_TEST_SUITE(movedlg);

static move_dlg_cb_t move_dlg_cb;

/** Create and destroy move dialog. */
PCUT_TEST(create_destroy)
{
	ui_t *ui;
	move_dlg_t *dlg = NULL;
	fmgt_flist_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = move_dlg_create(ui, flist, "foo", &dlg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dlg);

	move_dlg_destroy(dlg);

	fmgt_flist_destroy(flist);
	ui_destroy(ui);
}

/** Set callbacks for move dialog. */
PCUT_TEST(set_cb)
{
	ui_t *ui;
	move_dlg_t *dlg = NULL;
	fmgt_flist_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = move_dlg_create(ui, flist, "foo", &dlg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dlg);

	move_dlg_set_cb(dlg, &move_dlg_cb, NULL);

	move_dlg_destroy(dlg);

	fmgt_flist_destroy(flist);
	ui_destroy(ui);
}

PCUT_EXPORT(movedlg);
