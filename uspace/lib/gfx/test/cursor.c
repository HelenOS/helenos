/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gfx/context.h>
#include <gfx/cursor.h>
#include <pcut/pcut.h>
#include <mem.h>
#include <stdbool.h>

PCUT_INIT;

PCUT_TEST_SUITE(cursor);

static errno_t testgc_cursor_get_pos(void *, gfx_coord2_t *);
static errno_t testgc_cursor_set_pos(void *, gfx_coord2_t *);
static errno_t testgc_cursor_set_visible(void *, bool);

static gfx_context_ops_t ops = {
	.cursor_get_pos = testgc_cursor_get_pos,
	.cursor_set_pos = testgc_cursor_set_pos,
	.cursor_set_visible = testgc_cursor_set_visible
};

/** Test graphics context data */
typedef struct {
	errno_t rc;

	bool cursor_get_pos;
	gfx_coord2_t get_pos_pos;

	bool cursor_set_pos;
	gfx_coord2_t set_pos_pos;

	bool cursor_set_visible;
	bool set_visible_vis;
} test_gc_t;

/** Get hardware cursor position with error return */
PCUT_TEST(cursor_get_pos_failure)
{
	errno_t rc;
	gfx_coord2_t pos;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EIO;

	rc = gfx_cursor_get_pos(gc, &pos);
	PCUT_ASSERT_ERRNO_VAL(tgc.rc, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_get_pos);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Get hardware cursor position */
PCUT_TEST(cursor_get_pos_success)
{
	errno_t rc;
	gfx_coord2_t pos;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EOK;
	tgc.get_pos_pos.x = 1;
	tgc.get_pos_pos.y = 2;

	rc = gfx_cursor_get_pos(gc, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_get_pos);
	PCUT_ASSERT_INT_EQUALS(tgc.get_pos_pos.x, pos.x);
	PCUT_ASSERT_INT_EQUALS(tgc.get_pos_pos.y, pos.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set hardware cursor position with error return */
PCUT_TEST(cursor_set_pos_failure)
{
	errno_t rc;
	gfx_coord2_t pos;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EIO;
	pos.x = 1;
	pos.y = 2;

	rc = gfx_cursor_set_pos(gc, &pos);
	PCUT_ASSERT_ERRNO_VAL(tgc.rc, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_pos);
	PCUT_ASSERT_INT_EQUALS(pos.x, tgc.set_pos_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, tgc.set_pos_pos.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set hardware cursor position */
PCUT_TEST(cursor_set_pos_success)
{
	errno_t rc;
	gfx_coord2_t pos;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EOK;
	pos.x = 1;
	pos.y = 2;

	rc = gfx_cursor_set_pos(gc, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_pos);
	PCUT_ASSERT_INT_EQUALS(pos.x, tgc.set_pos_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, tgc.set_pos_pos.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set hardware cursor visibility with error return */
PCUT_TEST(cursor_set_visible_failure)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EIO;

	rc = gfx_cursor_set_visible(gc, true);
	PCUT_ASSERT_ERRNO_VAL(tgc.rc, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_visible);
	PCUT_ASSERT_TRUE(tgc.set_visible_vis);

	tgc.cursor_set_visible = false;

	rc = gfx_cursor_set_visible(gc, false);
	PCUT_ASSERT_ERRNO_VAL(tgc.rc, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_visible);
	PCUT_ASSERT_FALSE(tgc.set_visible_vis);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set hardware cursor visibility */
PCUT_TEST(cursor_set_visible_success)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EOK;

	rc = gfx_cursor_set_visible(gc, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_visible);
	PCUT_ASSERT_TRUE(tgc.set_visible_vis);

	tgc.cursor_set_visible = false;

	rc = gfx_cursor_set_visible(gc, false);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.cursor_set_visible);
	PCUT_ASSERT_FALSE(tgc.set_visible_vis);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_cursor_get_pos(void *arg, gfx_coord2_t *pos)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->cursor_get_pos = true;
	*pos = tgc->get_pos_pos;

	return tgc->rc;
}

static errno_t testgc_cursor_set_pos(void *arg, gfx_coord2_t *pos)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->cursor_set_pos = true;
	tgc->set_pos_pos = *pos;

	return tgc->rc;
}

static errno_t testgc_cursor_set_visible(void *arg, bool visible)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->cursor_set_visible = true;
	tgc->set_visible_vis = visible;

	return tgc->rc;
}

PCUT_EXPORT(cursor);
