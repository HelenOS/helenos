/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <io/kbd_event.h>
#include <pcut/pcut.h>

#include "../display.h"
#include "../types/display/ptd_event.h"
#include "../ievent.h"

PCUT_INIT;

PCUT_TEST_SUITE(ievent);

/* Test ds_ievent_init() and ds_ievent_fini() */
PCUT_TEST(ievent_init_fini)
{
	ds_display_t *disp;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_ievent_init(disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_ievent_fini(disp);
	ds_display_destroy(disp);
}

/* Test ds_ievent_post_kbd() */
PCUT_TEST(ievent_post_kbd)
{
	ds_display_t *disp;
	kbd_event_t kbd;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_ievent_init(disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	kbd.kbd_id = 0;
	kbd.type = KEY_PRESS;
	kbd.key = KC_ENTER;
	kbd.mods = 0;
	kbd.c = '\0';

	rc = ds_ievent_post_kbd(disp, &kbd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_ievent_fini(disp);
	ds_display_destroy(disp);
}

/* Test ds_ievent_post_ptd() */
PCUT_TEST(ievent_post_ptd)
{
	ds_display_t *disp;
	ptd_event_t ptd;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_ievent_init(disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ptd.pos_id = 0;
	ptd.type = PTD_MOVE;
	ptd.dmove.x = 0;
	ptd.dmove.y = 0;
	ptd.apos.x = 0;
	ptd.apos.y = 0;
	ptd.abounds.p0.x = 0;
	ptd.abounds.p0.y = 0;
	ptd.abounds.p1.x = 0;
	ptd.abounds.p1.y = 0;

	rc = ds_ievent_post_ptd(disp, &ptd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_ievent_fini(disp);
	ds_display_destroy(disp);
}

PCUT_EXPORT(ievent);
