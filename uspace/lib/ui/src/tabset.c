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

/** @addtogroup libui
 * @{
 */
/**
 * @file Tab set
 */

#include <adt/list.h>
#include <errno.h>
#include <gfx/render.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <ui/control.h>
#include <ui/tab.h>
#include <ui/tabset.h>
#include <ui/window.h>
#include "../private/tab.h"
#include "../private/tabset.h"
#include "../private/resource.h"

static void ui_tab_set_ctl_destroy(void *);
static errno_t ui_tab_set_ctl_paint(void *);
static ui_evclaim_t ui_tab_set_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_tab_set_ctl_pos_event(void *, pos_event_t *);

/** Tab set control ops */
ui_control_ops_t ui_tab_set_ops = {
	.destroy = ui_tab_set_ctl_destroy,
	.paint = ui_tab_set_ctl_paint,
	.kbd_event = ui_tab_set_ctl_kbd_event,
	.pos_event = ui_tab_set_ctl_pos_event
};

/** Create new tab set.
 *
 * @param res UI resource
 * @param rtabset Place to store pointer to new tab set
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_tab_set_create(ui_resource_t *res, ui_tab_set_t **rtabset)
{
	ui_tab_set_t *tabset;
	errno_t rc;

	tabset = calloc(1, sizeof(ui_tab_set_t));
	if (tabset == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_tab_set_ops, (void *) tabset, &tabset->control);
	if (rc != EOK) {
		free(tabset);
		return rc;
	}

	tabset->res = res;
	list_initialize(&tabset->tabs);
	*rtabset = tabset;
	return EOK;
}

/** Destroy tab set
 *
 * @param tabset Tab set or @c NULL
 */
void ui_tab_set_destroy(ui_tab_set_t *tabset)
{
	ui_tab_t *tab;

	if (tabset == NULL)
		return;

	/* Destroy tabs */
	tab = ui_tab_first(tabset);
	while (tab != NULL) {
		ui_tab_destroy(tab);
		tab = ui_tab_first(tabset);
	}

	ui_control_delete(tabset->control);
	free(tabset);
}

/** Get base control from tab set.
 *
 * @param tabset Tab set
 * @return Control
 */
ui_control_t *ui_tab_set_ctl(ui_tab_set_t *tabset)
{
	return tabset->control;
}

/** Set tab set rectangle.
 *
 * @param tabset Tab set
 * @param rect New tab set rectangle
 */
void ui_tab_set_set_rect(ui_tab_set_t *tabset, gfx_rect_t *rect)
{
	tabset->rect = *rect;
}

/** Paint tab set.
 *
 * @param tabset Tab set
 * @return EOK on success or an error code
 */
errno_t ui_tab_set_paint(ui_tab_set_t *tabset)
{
	ui_resource_t *res;
	ui_tab_t *tab;
	errno_t rc;

	res = tabset->res;

	if (tabset->selected != NULL) {
		rc = ui_tab_paint_body_frame(tabset->selected);
		if (rc != EOK)
			goto error;
	}

	tab = ui_tab_first(tabset);
	while (tab != NULL) {
		rc = ui_tab_paint(tab);
		if (rc != EOK)
			return rc;

		tab = ui_tab_next(tab);
	}

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Select or deselect tab from tab set.
 *
 * Select @a tab. If @a tab is @c NULL or it is already selected,
 * then select none.
 *
 * @param tabset Tab set
 * @param tab Tab to select (or deselect if selected) or @c NULL
 */
void ui_tab_set_select(ui_tab_set_t *tabset, ui_tab_t *tab)
{
	tabset->selected = tab;
	(void) ui_tab_set_paint(tabset);
}

/** Handle tab set keyboard event.
 *
 * @param tabset Tab set
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_tab_set_kbd_event(ui_tab_set_t *tabset, kbd_event_t *event)
{
	ui_tab_t *tab;
	ui_evclaim_t claim;

	tab = ui_tab_first(tabset);
	while (tab != NULL) {
		claim = ui_tab_kbd_event(tab, event);
		if (claim == ui_claimed)
			return ui_claimed;

		tab = ui_tab_next(tab);
	}

	return ui_unclaimed;
}

/** Handle tab set position event.
 *
 * @param tabset Tab set
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_tab_set_pos_event(ui_tab_set_t *tabset, pos_event_t *event)
{
	ui_tab_t *tab;
	ui_evclaim_t claim;

	tab = ui_tab_first(tabset);
	while (tab != NULL) {
		claim = ui_tab_pos_event(tab, event);
		if (claim == ui_claimed)
			return ui_claimed;

		tab = ui_tab_next(tab);
	}

	return ui_unclaimed;
}

/** Destroy tab set control.
 *
 * @param arg Argument (ui_tab_set_t *)
 */
static void ui_tab_set_ctl_destroy(void *arg)
{
	ui_tab_set_t *tabset = (ui_tab_set_t *) arg;

	ui_tab_set_destroy(tabset);
}

/** Paint tab set control.
 *
 * @param arg Argument (ui_tab_set_t *)
 * @return EOK on success or an error code
 */
static errno_t ui_tab_set_ctl_paint(void *arg)
{
	ui_tab_set_t *tabset = (ui_tab_set_t *) arg;

	return ui_tab_set_paint(tabset);
}

/** Handle tab set control keyboard event.
 *
 * @param arg Argument (ui_tab_set_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
static ui_evclaim_t ui_tab_set_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_tab_set_t *tabset = (ui_tab_set_t *) arg;

	return ui_tab_set_kbd_event(tabset, event);
}

/** Handle tab set control position event.
 *
 * @param arg Argument (ui_tab_set_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
static ui_evclaim_t ui_tab_set_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_tab_set_t *tabset = (ui_tab_set_t *) arg;

	return ui_tab_set_pos_event(tabset, event);
}

/** @}
 */
