/*
 * Copyright (c) 2020 Jiri Svoboda
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

#include "../cursor.h"
#include "../cursimg.h"
#include "../display.h"

PCUT_INIT;

PCUT_TEST_SUITE(cursor);

/** Test ds_cursor_create(), ds_cursor_destroy(). */
PCUT_TEST(cursor_create_destroy)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

/** Test ds_cursor_paint(). */
PCUT_TEST(cursor_paint)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

/** Test ds_cursor_get_rect() */
PCUT_TEST(cursor_get_rect)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

PCUT_EXPORT(cursor);
