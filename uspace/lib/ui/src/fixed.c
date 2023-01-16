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
 * @file Fixed layout
 */

#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <ui/control.h>
#include <ui/fixed.h>
#include "../private/control.h"
#include "../private/fixed.h"

static void ui_fixed_ctl_destroy(void *);
static errno_t ui_fixed_ctl_paint(void *);
static ui_evclaim_t ui_fixed_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_fixed_ctl_pos_event(void *, pos_event_t *);
static void ui_fixed_ctl_unfocus(void *, unsigned);

/** Push button control ops */
ui_control_ops_t ui_fixed_ops = {
	.destroy = ui_fixed_ctl_destroy,
	.paint = ui_fixed_ctl_paint,
	.kbd_event = ui_fixed_ctl_kbd_event,
	.pos_event = ui_fixed_ctl_pos_event,
	.unfocus = ui_fixed_ctl_unfocus
};

/** Create new fixed layout.
 *
 * @param rfixed Place to store pointer to new fixed layout
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_fixed_create(ui_fixed_t **rfixed)
{
	ui_fixed_t *fixed;
	errno_t rc;

	fixed = calloc(1, sizeof(ui_fixed_t));
	if (fixed == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_fixed_ops, (void *) fixed, &fixed->control);
	if (rc != EOK) {
		free(fixed);
		return rc;
	}

	list_initialize(&fixed->elem);
	*rfixed = fixed;
	return EOK;
}

/** Destroy fixed layout.
 *
 * @param fixed Fixed layout or @c NULL
 */
void ui_fixed_destroy(ui_fixed_t *fixed)
{
	ui_fixed_elem_t *elem;
	ui_control_t *control;

	if (fixed == NULL)
		return;

	elem = ui_fixed_first(fixed);
	while (elem != NULL) {
		control = elem->control;
		ui_fixed_remove(fixed, control);
		ui_control_destroy(control);

		elem = ui_fixed_first(fixed);
	}

	ui_control_delete(fixed->control);
	free(fixed);
}

/** Get base control from fixed layout.
 *
 * @param fixed Fixed layout
 * @return Control
 */
ui_control_t *ui_fixed_ctl(ui_fixed_t *fixed)
{
	return fixed->control;
}

/** Add control to fixed layout.
 *
 * @param fixed Fixed layout
 * @param control Control
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_fixed_add(ui_fixed_t *fixed, ui_control_t *control)
{
	ui_fixed_elem_t *elem;

	elem = calloc(1, sizeof(ui_fixed_elem_t));
	if (elem == NULL)
		return ENOMEM;

	elem->fixed = fixed;
	elem->control = control;
	control->elemp = (void *) elem;
	list_append(&elem->lelems, &fixed->elem);

	return EOK;
}

/** Remove control from fixed layout.
 *
 * @param fixed Fixed layout
 * @param control Control
 */
void ui_fixed_remove(ui_fixed_t *fixed, ui_control_t *control)
{
	ui_fixed_elem_t *elem;

	elem = (ui_fixed_elem_t *) control->elemp;
	assert(elem->fixed == fixed);

	list_remove(&elem->lelems);
	control->elemp = NULL;

	free(elem);
}

/** Get first element of fixed layout.
 *
 * @param fixed Fixed layout
 * @return First element or @c NULL
 */
ui_fixed_elem_t *ui_fixed_first(ui_fixed_t *fixed)
{
	link_t *link;

	link = list_first(&fixed->elem);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_fixed_elem_t, lelems);
}

/** Get next element of fixed layout.
 *
 * @param cur Current element
 * @return Next element or @c NULL
 */
ui_fixed_elem_t *ui_fixed_next(ui_fixed_elem_t *cur)
{
	link_t *link;

	link = list_next(&cur->lelems, &cur->fixed->elem);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_fixed_elem_t, lelems);
}

/** Paint fixed layout.
 *
 * @param fixed Fixed layout
 * @return EOK on success or an error code
 */
errno_t ui_fixed_paint(ui_fixed_t *fixed)
{
	ui_fixed_elem_t *elem;
	errno_t rc;

	elem = ui_fixed_first(fixed);
	while (elem != NULL) {
		rc = ui_control_paint(elem->control);
		if (rc != EOK)
			return rc;

		elem = ui_fixed_next(elem);
	}

	return EOK;
}

/** Handle fixed layout keyboard event.
 *
 * @param fixed Fixed layout
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_fixed_kbd_event(ui_fixed_t *fixed, kbd_event_t *event)
{
	ui_fixed_elem_t *elem;
	ui_evclaim_t claimed;

	elem = ui_fixed_first(fixed);
	while (elem != NULL) {
		claimed = ui_control_kbd_event(elem->control, event);
		if (claimed == ui_claimed)
			return ui_claimed;

		elem = ui_fixed_next(elem);
	}

	return ui_unclaimed;
}

/** Handle fixed layout position event.
 *
 * @param fixed Fixed layout
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_fixed_pos_event(ui_fixed_t *fixed, pos_event_t *event)
{
	ui_fixed_elem_t *elem;
	ui_evclaim_t claimed;

	elem = ui_fixed_first(fixed);
	while (elem != NULL) {
		claimed = ui_control_pos_event(elem->control, event);
		if (claimed == ui_claimed)
			return ui_claimed;

		elem = ui_fixed_next(elem);
	}

	return ui_unclaimed;
}

/** Handle fixed layout window unfocus notification.
 *
 * @param fixed Fixed layout
 * @param nfocus Number of remaining foci
 */
void ui_fixed_unfocus(ui_fixed_t *fixed, unsigned nfocus)
{
	ui_fixed_elem_t *elem;

	elem = ui_fixed_first(fixed);
	while (elem != NULL) {
		ui_control_unfocus(elem->control, nfocus);

		elem = ui_fixed_next(elem);
	}
}

/** Destroy fixed layout control.
 *
 * @param arg Argument (ui_fixed_t *)
 */
void ui_fixed_ctl_destroy(void *arg)
{
	ui_fixed_t *fixed = (ui_fixed_t *) arg;

	ui_fixed_destroy(fixed);
}

/** Paint fixed layout control.
 *
 * @param arg Argument (ui_fixed_t *)
 * @return EOK on success or an error code
 */
errno_t ui_fixed_ctl_paint(void *arg)
{
	ui_fixed_t *fixed = (ui_fixed_t *) arg;

	return ui_fixed_paint(fixed);
}

/** Handle fixed layout control keyboard event.
 *
 * @param arg Argument (ui_fixed_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_fixed_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_fixed_t *fixed = (ui_fixed_t *) arg;

	return ui_fixed_kbd_event(fixed, event);
}

/** Handle fixed layout control position event.
 *
 * @param arg Argument (ui_fixed_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_fixed_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_fixed_t *fixed = (ui_fixed_t *) arg;

	return ui_fixed_pos_event(fixed, event);
}

/** Handle fixed layout control window unfocus notification.
 *
 * @param arg Argument (ui_fixed_t *)
 * @param nfocus Number of remaining foci
 */
void ui_fixed_ctl_unfocus(void *arg, unsigned nfocus)
{
	ui_fixed_t *fixed = (ui_fixed_t *) arg;

	ui_fixed_unfocus(fixed, nfocus);
}

/** @}
 */
