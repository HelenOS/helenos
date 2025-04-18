/*
 * Copyright (c) 2024
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

/** @addtogroup libui
 * @{
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <ui/clock.h>
#include <ui/paint.h>
#include <ui/resource.h>

#define CLOCK_HAND_WIDTH 2
#define CLOCK_HOUR_HAND_LENGTH 0.6
#define CLOCK_MINUTE_HAND_LENGTH 0.8
#define CLOCK_SECOND_HAND_LENGTH 0.9

static void ui_clock_ctl_destroy(void *);
static errno_t ui_clock_ctl_paint(void *);

/** Clock control ops */
ui_control_ops_t ui_clock_ops = {
	.destroy = ui_clock_ctl_destroy,
	.paint = ui_clock_ctl_paint
};

/** Create clock widget.
 *
 * @param resource UI resource
 * @param rclock Place to store pointer to new clock
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_clock_create(ui_resource_t *resource, ui_clock_t **rclock)
{
	ui_clock_t *clock;
	errno_t rc;

	clock = calloc(1, sizeof(ui_clock_t));
	if (clock == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_clock_ops, (void *) clock, &clock->control);
	if (rc != EOK) {
		free(clock);
		return rc;
	}

	clock->res = resource;
	*rclock = clock;
	return EOK;
}

/** Destroy clock widget.
 *
 * @param clock Clock widget or @c NULL
 */
void ui_clock_destroy(ui_clock_t *clock)
{
	if (clock == NULL)
		return;

	ui_control_delete(clock->control);
	free(clock);
}

/** Get base control from clock.
 *
 * @param clock Clock
 * @return Control
 */
ui_control_t *ui_clock_ctl(ui_clock_t *clock)
{
	return clock->control;
}

/** Set clock rectangle.
 *
 * @param clock Clock
 * @param rect New clock rectangle
 */
void ui_clock_set_rect(ui_clock_t *clock, gfx_rect_t *rect)
{
	clock->rect = *rect;
}

/** Draw clock hand.
 *
 * @param gc Graphic context
 * @param center Center point
 * @param angle Angle in radians
 * @param length Length as fraction of radius
 * @param width Width of hand
 * @param color Color to use
 * @return EOK on success or an error code
 */
static errno_t ui_clock_draw_hand(gfx_context_t *gc, gfx_coord2_t *center,
    double angle, double length, gfx_coord_t width, gfx_color_t *color)
{
	gfx_coord_t radius;
	gfx_coord2_t end;
	gfx_rect_t rect;
	errno_t rc;

	/* Calculate radius as minimum of width/height */
	radius = min(clock->rect.p1.x - clock->rect.p0.x,
	    clock->rect.p1.y - clock->rect.p0.y) / 2;

	/* Calculate end point */
	end.x = center->x + (gfx_coord_t)(radius * length * sin(angle));
	end.y = center->y - (gfx_coord_t)(radius * length * cos(angle));

	/* Draw hand as a rectangle */
	rect.p0.x = center->x - width/2;
	rect.p0.y = center->y - width/2;
	rect.p1.x = end.x + width/2;
	rect.p1.y = end.y + width/2;

	rc = gfx_set_color(gc, color);
	if (rc != EOK)
		return rc;

	return gfx_fill_rect(gc, &rect);
}

/** Paint clock.
 *
 * @param clock Clock
 * @return EOK on success or an error code
 */
errno_t ui_clock_paint(ui_clock_t *clock)
{
	gfx_coord2_t center;
	gfx_coord_t radius;
	double hour_angle, min_angle, sec_angle;
	errno_t rc;

	/* Calculate center and radius */
	center.x = (clock->rect.p0.x + clock->rect.p1.x) / 2;
	center.y = (clock->rect.p0.y + clock->rect.p1.y) / 2;
	radius = min(clock->rect.p1.x - clock->rect.p0.x,
	    clock->rect.p1.y - clock->rect.p0.y) / 2;

	/* Paint clock face */
	rc = gfx_set_color(clock->res->gc, clock->res->wnd_face_color);
	if (rc != EOK)
		return rc;

	rc = ui_paint_filled_circle(clock->res->gc, &center, radius,
	    ui_fcircle_entire);
	if (rc != EOK)
		return rc;

	/* Paint clock frame */
	rc = ui_paint_outset_frame(clock->res, &clock->rect, NULL);
	if (rc != EOK)
		return rc;

	/* Calculate hand angles */
	hour_angle = (clock->hour % 12 + clock->minute / 60.0) * M_PI / 6;
	min_angle = clock->minute * M_PI / 30;
	sec_angle = clock->second * M_PI / 30;

	/* Draw hour hand */
	rc = ui_clock_draw_hand(clock->res->gc, &center, hour_angle,
	    CLOCK_HOUR_HAND_LENGTH, CLOCK_HAND_WIDTH * 2,
	    clock->res->btn_text_color);
	if (rc != EOK)
		return rc;

	/* Draw minute hand */
	rc = ui_clock_draw_hand(clock->res->gc, &center, min_angle,
	    CLOCK_MINUTE_HAND_LENGTH, CLOCK_HAND_WIDTH,
	    clock->res->btn_text_color);
	if (rc != EOK)
		return rc;

	/* Draw second hand */
	rc = ui_clock_draw_hand(clock->res->gc, &center, sec_angle,
	    CLOCK_SECOND_HAND_LENGTH, CLOCK_HAND_WIDTH,
	    clock->res->wnd_highlight_color);
	if (rc != EOK)
		return rc;

	return gfx_update(clock->res->gc);
}

/** Update clock time.
 *
 * @param clock Clock
 */
void ui_clock_update_time(ui_clock_t *clock)
{
	time_t now;
	struct tm *tm;

	time(&now);
	tm = localtime(&now);

	clock->hour = tm->tm_hour;
	clock->minute = tm->tm_min;
	clock->second = tm->tm_sec;
}

/** Destroy clock control.
 *
 * @param arg Argument (ui_clock_t *)
 */
static void ui_clock_ctl_destroy(void *arg)
{
	ui_clock_t *clock = (ui_clock_t *) arg;

	ui_clock_destroy(clock);
}

/** Paint clock control.
 *
 * @param arg Argument (ui_clock_t *)
 * @return EOK on success or an error code
 */
static errno_t ui_clock_ctl_paint(void *arg)
{
	ui_clock_t *clock = (ui_clock_t *) arg;

	return ui_clock_paint(clock);
}

/** @}
 */ 