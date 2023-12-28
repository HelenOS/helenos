/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup taskbar
 * @{
 */
/** @file Taskbar clock.
 *
 * Displays the current time in an inset frame.
 */

#include <errno.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <stdio.h>
#include <task.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include "clock.h"

static void taskbar_clock_ctl_destroy(void *);
static errno_t taskbar_clock_ctl_paint(void *);
static ui_evclaim_t taskbar_clock_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t taskbar_clock_ctl_pos_event(void *, pos_event_t *);
static void taskbar_clock_timer(void *);

/** Taskbar clock control ops */
static ui_control_ops_t taskbar_clock_ctl_ops = {
	.destroy = taskbar_clock_ctl_destroy,
	.paint = taskbar_clock_ctl_paint,
	.kbd_event = taskbar_clock_ctl_kbd_event,
	.pos_event = taskbar_clock_ctl_pos_event
};

/** Create taskbar clock.
 *
 * @param window Containing window
 * @param rclock Place to store pointer to new clock
 * @return EOK on success or an error code
 */
errno_t taskbar_clock_create(ui_window_t *window, taskbar_clock_t **rclock)
{
	taskbar_clock_t *clock;
	errno_t rc;

	clock = calloc(1, sizeof(taskbar_clock_t));
	if (clock == NULL)
		return ENOMEM;

	rc = ui_control_new(&taskbar_clock_ctl_ops, (void *)clock,
	    &clock->control);
	if (rc != EOK) {
		free(clock);
		return rc;
	}

	clock->timer = fibril_timer_create(NULL);
	if (clock->timer == NULL) {
		rc = ENOMEM;
		goto error;
	}

	fibril_mutex_initialize(&clock->lock);
	fibril_condvar_initialize(&clock->timer_done_cv);
	fibril_timer_set(clock->timer, 1000000, taskbar_clock_timer, clock);

	clock->window = window;
	*rclock = clock;
	return EOK;
error:
	ui_control_delete(clock->control);
	free(clock);
	return rc;
}

/** Destroy taskbar clock.
 *
 * @param clock Taskbar clock
 */
void taskbar_clock_destroy(taskbar_clock_t *clock)
{
	/*
	 * Signal to the timer that we are cleaning up. If the timer handler
	 * misses it and sets the timer again, we will clear that active
	 * timer and be done (and if we were even slower and the timer
	 * fired again, it's the same situation as before.
	 */
	fibril_mutex_lock(&clock->lock);
	clock->timer_cleanup = true;
	fibril_mutex_unlock(&clock->lock);

	/* If we catch the timer while it's active, there's nothing to do. */
	if (fibril_timer_clear(clock->timer) != fts_active) {
		/* Need to wait for timer handler to finish */
		fibril_mutex_lock(&clock->lock);
		while (clock->timer_done == false)
			fibril_condvar_wait(&clock->timer_done_cv, &clock->lock);
		fibril_mutex_unlock(&clock->lock);
	}

	fibril_timer_destroy(clock->timer);
	ui_control_delete(clock->control);
	free(clock);
}

static errno_t taskbar_clock_get_text(taskbar_clock_t *clock, char *buf,
    size_t bsize)
{
	struct timespec ts;
	struct tm tm;
	errno_t rc;

	getrealtime(&ts);
	rc = time_utc2tm(ts.tv_sec, &tm);
	if (rc != EOK)
		return rc;

	snprintf(buf, bsize, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min,
	    tm.tm_sec);
	return EOK;
}

/** Paint taskbar clock.
 *
 * @param clock Taskbar clock
 */
errno_t taskbar_clock_paint(taskbar_clock_t *clock)
{
	gfx_context_t *gc = ui_window_get_gc(clock->window);
	ui_resource_t *res = ui_window_get_res(clock->window);
	ui_t *ui = ui_window_get_ui(clock->window);
	char buf[10];
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_rect_t irect;
	errno_t rc;

	if (!ui_is_textmode(ui)) {
		/* Paint frame */
		rc = ui_paint_inset_frame(res, &clock->rect, &irect);
		if (rc != EOK)
			goto error;
	} else {
		irect = clock->rect;
	}

	rc = gfx_set_color(gc, ui_resource_get_wnd_face_color(res));
	if (rc != EOK)
		goto error;

	/* Fill background */
	rc = gfx_fill_rect(gc, &irect);
	if (rc != EOK)
		goto error;

	pos.x = (irect.p0.x + irect.p1.x) / 2;
	pos.y = (irect.p0.y + irect.p1.y) / 2;

	gfx_text_fmt_init(&fmt);
	fmt.font = ui_resource_get_font(res);
	fmt.color = ui_resource_get_wnd_text_color(res);
	fmt.halign = gfx_halign_center;
	fmt.valign = gfx_valign_center;

	rc = taskbar_clock_get_text(clock, buf, sizeof(buf));
	if (rc != EOK)
		goto error;

	rc = gfx_puttext(&pos, &fmt, buf);
	if (rc != EOK)
		goto error;

	rc = gfx_update(gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Handle taskbar clock keyboard event.
 *
 * @param clock Taskbar clock
 * @param event Keyboard event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t taskbar_clock_kbd_event(taskbar_clock_t *clock, kbd_event_t *event)
{
	return ui_unclaimed;
}

/** Handle taskbar clock position event.
 *
 * @param clock Taskbar clock
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t taskbar_clock_pos_event(taskbar_clock_t *clock, pos_event_t *event)
{
	gfx_coord2_t pos;

	pos.x = event->hpos;
	pos.y = event->vpos;
	if (!gfx_pix_inside_rect(&pos, &clock->rect))
		return ui_unclaimed;

	return ui_claimed;
}

/** Get base control for taskbar clock.
 *
 * @param clock Taskbar clock
 * @return Base UI control
 */
ui_control_t *taskbar_clock_ctl(taskbar_clock_t *clock)
{
	return clock->control;
}

/** Set taskbar clock rectangle.
 *
 * @param clock Taskbar clock
 * @param rect Rectangle
 */
void taskbar_clock_set_rect(taskbar_clock_t *clock, gfx_rect_t *rect)
{
	gfx_rect_t irect;

	clock->rect = *rect;

	irect.p0.x = clock->rect.p0.x + 1;
	irect.p0.y = clock->rect.p0.y + 1;
	irect.p1.x = clock->rect.p1.x;
	irect.p1.y = clock->rect.p1.y - 1;

	(void)irect;
}

/** Destroy clock control.
 *
 * @param arg Argument (taskbar_clock_t *)
 */
void taskbar_clock_ctl_destroy(void *arg)
{
	taskbar_clock_t *clock = (taskbar_clock_t *) arg;

	taskbar_clock_destroy(clock);
}

/** Paint taskbar clock control.
 *
 * @param arg Argument (taskbar_clock_t *)
 * @return EOK on success or an error code
 */
errno_t taskbar_clock_ctl_paint(void *arg)
{
	taskbar_clock_t *clock = (taskbar_clock_t *) arg;

	return taskbar_clock_paint(clock);
}

/** Handle taskbar clock control keyboard event.
 *
 * @param arg Argument (taskbar_clock_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t taskbar_clock_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	taskbar_clock_t *clock = (taskbar_clock_t *) arg;

	return taskbar_clock_kbd_event(clock, event);
}

/** Handle taskbar clock control position event.
 *
 * @param arg Argument (taskbar_clock_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t taskbar_clock_ctl_pos_event(void *arg, pos_event_t *event)
{
	taskbar_clock_t *clock = (taskbar_clock_t *) arg;

	return taskbar_clock_pos_event(clock, event);
}

/** Taskbar clock timer handler.
 *
 * @param arg Argument (taskbar_clock_t *)
 */
static void taskbar_clock_timer(void *arg)
{
	taskbar_clock_t *clock = (taskbar_clock_t *) arg;
	ui_t *ui;

	ui = ui_window_get_ui(clock->window);
	ui_lock(ui);

	fibril_mutex_lock(&clock->lock);
	if (!ui_is_suspended(ui_window_get_ui(clock->window)))
		(void) taskbar_clock_paint(clock);

	if (!clock->timer_cleanup) {
		fibril_timer_set(clock->timer, 1000000, taskbar_clock_timer,
		    clock);
	} else {
		/* Acknowledge timer cleanup */
		clock->timer_done = true;
		fibril_condvar_signal(&clock->timer_done_cv);
	}

	fibril_mutex_unlock(&clock->lock);
	ui_unlock(ui);
}

/** @}
 */
