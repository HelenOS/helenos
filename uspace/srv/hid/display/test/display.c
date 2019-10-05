/*
 * Copyright (c) 2019 Jiri Svoboda
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
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>

#include "../display.h"
#include "../window.h"

PCUT_INIT;

PCUT_TEST_SUITE(display);

/** Display creation and destruction. */
PCUT_TEST(display_create_destroy)
{
	ds_display_t *disp;
	errno_t rc;

	rc = ds_display_create(&disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_display_destroy(disp);
}

/** Basic window operation. */
PCUT_TEST(display_window)
{
	ds_display_t *disp;
	ds_window_t *wnd;
	ds_window_t *w0, *w1, *w2;
	errno_t rc;

	rc = ds_display_create(&disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(disp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	w0 = ds_display_first_window(disp);
	PCUT_ASSERT_EQUALS(w0, wnd);

	w1 = ds_display_next_window(w0);
	PCUT_ASSERT_NULL(w1);

	w2 = ds_display_find_window(disp, wnd->id);
	PCUT_ASSERT_EQUALS(w2, wnd);

	ds_window_delete(wnd);
	ds_display_destroy(disp);
}

PCUT_EXPORT(display);
