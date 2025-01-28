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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/label.h>
#include <ui/resource.h>
#include "../private/label.h"
#include "../private/testgc.h"

PCUT_INIT;

PCUT_TEST_SUITE(label);

typedef struct {
	bool clicked;
} test_cb_resp_t;

/** Create and destroy label */
PCUT_TEST(create_destroy)
{
	ui_label_t *label = NULL;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(label);

	ui_label_destroy(label);
}

/** ui_label_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_label_destroy(NULL);
}

/** ui_label_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_label_t *label;
	ui_control_t *control;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_label_ctl(label);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set label rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_label_t *label;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_label_set_rect(label, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, label->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, label->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, label->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, label->rect.p1.y);

	ui_label_destroy(label);
}

/** Set label text horizontal alignment sets internal field */
PCUT_TEST(set_halign)
{
	ui_label_t *label;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_label_set_halign(label, gfx_halign_left);
	PCUT_ASSERT_EQUALS(gfx_halign_left, label->halign);
	ui_label_set_halign(label, gfx_halign_center);
	PCUT_ASSERT_EQUALS(gfx_halign_center, label->halign);

	ui_label_destroy(label);
}

/** Set label rectangle sets internal field */
PCUT_TEST(set_text)
{
	ui_label_t *label;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_label_set_rect(label, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, label->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, label->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, label->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, label->rect.p1.y);

	ui_label_destroy(label);
}

/** Paint label */
PCUT_TEST(paint)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_label_t *label;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_label_create(resource, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_label_paint(label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_label_destroy(label);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_EXPORT(label);
