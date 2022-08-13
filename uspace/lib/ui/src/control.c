/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file UI control
 */

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <ui/control.h>
#include "../private/control.h"

/** Allocate new UI control.
 *
 * @param ops Control ops
 * @param ext Control extended data
 * @param rcontrol Place to store pointer to new control
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_control_new(ui_control_ops_t *ops, void *ext,
    ui_control_t **rcontrol)
{
	ui_control_t *control;

	control = calloc(1, sizeof(ui_control_t));
	if (control == NULL)
		return ENOMEM;

	control->ops = ops;
	control->ext = ext;
	*rcontrol = control;
	return EOK;
}

/** Delete UI control.
 *
 * Deletes the base control (not the extended data).
 *
 * @param control UI control or @c NULL
 */
void ui_control_delete(ui_control_t *control)
{
	if (control == NULL)
		return;

	free(control);
}

/** Destroy UI control.
 *
 * Run the virtual control destructor (destroy complete control including
 * extended data).
 *
 * @param control Control or @c NULL
 */
void ui_control_destroy(ui_control_t *control)
{
	if (control == NULL)
		return;

	return control->ops->destroy(control->ext);
}

/** Deliver keyboard event to UI control.
 *
 * @param control Control
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_control_kbd_event(ui_control_t *control, kbd_event_t *event)
{
	if (control->ops->kbd_event != NULL)
		return control->ops->kbd_event(control->ext, event);
	else
		return ui_unclaimed;
}

/** Paint UI control.
 *
 * @param control Control
 * @return EOK on success or an error code
 */
errno_t ui_control_paint(ui_control_t *control)
{
	return control->ops->paint(control->ext);
}

/** Deliver position event to UI control.
 *
 * @param control Control
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_control_pos_event(ui_control_t *control, pos_event_t *event)
{
	return control->ops->pos_event(control->ext, event);
}

/** Inform UI control that window has been unfocused.
 *
 * @param control Control
 */
void ui_control_unfocus(ui_control_t *control)
{
	if (control->ops->unfocus != NULL)
		control->ops->unfocus(control->ext);
}

/** @}
 */
