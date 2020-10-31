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

/** Create new fixed layout.
 *
 * @param rfixed Place to store pointer to new fixed layout
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_fixed_create(ui_fixed_t **rfixed)
{
	ui_fixed_t *fixed;

	fixed = calloc(1, sizeof(ui_fixed_t));
	if (fixed == NULL)
		return ENOMEM;

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
	if (fixed == NULL)
		return;

	assert(list_empty(&fixed->elem));
	free(fixed);
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

/** @}
 */
