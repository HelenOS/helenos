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
 * @file Clickmatic.
 *
 * Clickmatic is used to periodically generate events in particular cases
 * when a mouse button is held down, such as when holding the button or
 * trough of a scrollbar.
 */

#include <fibril_synch.h>
#include <stdlib.h>
#include <ui/clickmatic.h>
#include <ui/ui.h>
#include "../private/clickmatic.h"

enum {
	/** Initial clickmatic delay in MS */
	clickmatic_delay_ms = 500,
	/** Clickmatic repeat rate in clicks per second */
	clickmatic_rate = 10
};

static void ui_clickmatic_timer_fun(void *);

/** Create clickmatic.
 *
 * @param ui UI
 * @param rclickmatic Place to store pointer to new clickmatic
 * @return EOK on success or an error code
 */
errno_t ui_clickmatic_create(ui_t *ui, ui_clickmatic_t **rclickmatic)
{
	ui_clickmatic_t *clickmatic;

	clickmatic = calloc(1, sizeof(ui_clickmatic_t));
	if (clickmatic == NULL)
		return ENOMEM;

	clickmatic->ui = ui;
	clickmatic->timer = fibril_timer_create(NULL);
	if (clickmatic->timer == NULL) {
		free(clickmatic);
		return ENOMEM;
	}

	*rclickmatic = clickmatic;
	return EOK;
}

/** Set clickmatick callbacks.
 *
 * @param clickmatic Clickmatic
 * @param cb Callbacks
 * @param arg Argument to callbacks
 */
void ui_clickmatic_set_cb(ui_clickmatic_t *clickmatic, ui_clickmatic_cb_t *cb,
    void *arg)
{
	clickmatic->cb = cb;
	clickmatic->arg = arg;
}

/** Destroy clickmatic.
 *
 * @param clickmatic Clickmatic or @c NULL
 */
void ui_clickmatic_destroy(ui_clickmatic_t *clickmatic)
{
	if (clickmatic == NULL)
		return;

	fibril_timer_destroy(clickmatic->timer);
	free(clickmatic);
}

/** Activate clickmatic.
 *
 * This generates one click event, then starts repeating after delay.
 *
 * @param clickmatic Clickmatic
 */
void ui_clickmatic_press(ui_clickmatic_t *clickmatic)
{
	ui_clickmatic_clicked(clickmatic);

	fibril_timer_set(clickmatic->timer, 1000 * clickmatic_delay_ms,
	    ui_clickmatic_timer_fun, (void *)clickmatic);
}

/** Deactivate clickmatic.
 *
 * Stops generating events.
 *
 * @param clickmatic Clickmatic
 */
void ui_clickmatic_release(ui_clickmatic_t *clickmatic)
{
	(void) fibril_timer_clear(clickmatic->timer);
}

/** Clickmatic clicked event.
 *
 * @param clickmatic Clickmatic
 */
void ui_clickmatic_clicked(ui_clickmatic_t *clickmatic)
{
	if (clickmatic->cb != NULL && clickmatic->cb->clicked != NULL)
		clickmatic->cb->clicked(clickmatic, clickmatic->arg);
}

/** Clickmatic timer function.
 *
 * @param arg Argument (ui_clickmatic_t *)
 */
static void ui_clickmatic_timer_fun(void *arg)
{
	ui_clickmatic_t *clickmatic = (ui_clickmatic_t *)arg;

	/*
	 * Because we are operating in a different fibril, we must lock
	 * the UI to ensure mutual exclusion with normal UI events
	 */

	ui_lock(clickmatic->ui);
	ui_clickmatic_clicked(clickmatic);
	ui_unlock(clickmatic->ui);

	fibril_timer_set(clickmatic->timer, 1000000 / clickmatic_rate,
	    ui_clickmatic_timer_fun, (void *)clickmatic);
}

/** @}
 */
