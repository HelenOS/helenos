/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/image.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include "../private/dummygc.h"
#include "../private/image.h"

PCUT_INIT;

PCUT_TEST_SUITE(image);

/** Create and destroy image */
PCUT_TEST(create_destroy)
{
	ui_image_t *image = NULL;
	gfx_rect_t brect;
	errno_t rc;

	rc = ui_image_create(NULL, NULL, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	ui_image_destroy(image);
}

/** ui_image_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_image_destroy(NULL);
}

/** ui_image_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_image_t *image = NULL;
	ui_control_t *control;
	gfx_rect_t brect;
	errno_t rc;

	rc = ui_image_create(NULL, NULL, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	control = ui_image_ctl(image);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set image rectangle sets internal field */
PCUT_TEST(set_rect)
{
	errno_t rc;
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_image_t *image = NULL;
	gfx_rect_t brect;
	gfx_rect_t rect;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_image_create(resource, NULL, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_image_set_rect(image, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, image->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, image->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, image->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, image->rect.p1.y);

	ui_image_destroy(image);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Set image flags sets internal field */
PCUT_TEST(set_flags)
{
	ui_image_t *image = NULL;
	gfx_rect_t brect;
	errno_t rc;

	rc = ui_image_create(NULL, NULL, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	PCUT_ASSERT_INT_EQUALS(0, image->flags);

	ui_image_set_flags(image, ui_imgf_frame);
	PCUT_ASSERT_INT_EQUALS(ui_imgf_frame, image->flags);

	ui_image_destroy(image);
}

/** Set image bitmap */
PCUT_TEST(set_bmp)
{
	ui_image_t *image = NULL;
	gfx_rect_t brect;
	gfx_rect_t rect;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_image_create(resource, NULL, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_image_set_rect(image, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, image->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, image->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, image->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, image->rect.p1.y);

	gfx_bitmap_params_init(&params);
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_image_set_bmp(image, bitmap, &brect);
	PCUT_ASSERT_EQUALS(bitmap, image->bitmap);

	rc = ui_image_paint(image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_image_destroy(image);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Paint image */
PCUT_TEST(paint)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	ui_resource_t *resource = NULL;
	ui_image_t *image = NULL;
	gfx_rect_t brect;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	gfx_bitmap_params_init(&params);
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_image_create(resource, bitmap, &brect, &image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(image);

	rc = ui_image_paint(image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check that we can paint image after setting bitmap to NULL */

	ui_image_set_bmp(image, NULL, &brect);

	rc = ui_image_paint(image);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_image_destroy(image);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

PCUT_EXPORT(image);
