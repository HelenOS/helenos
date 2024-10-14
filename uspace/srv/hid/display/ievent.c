/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/** @file Input event queue
 */

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/kbd_event.h>
#include <stdlib.h>
#include "display.h"
#include "ievent.h"

/** Post keyboard event to input event queue.
 *
 * @param disp Display
 * @param kbd Keyboard event
 *
 * @return EOK on success or an error code
 */
errno_t ds_ievent_post_kbd(ds_display_t *disp, kbd_event_t *kbd)
{
	ds_ievent_t *ievent;

	ievent = calloc(1, sizeof(ds_ievent_t));
	if (ievent == NULL)
		return ENOMEM;

	ievent->display = disp;
	ievent->etype = det_kbd;
	ievent->ev.kbd = *kbd;

	list_append(&ievent->lievents, &disp->ievents);
	fibril_condvar_signal(&disp->ievent_cv);

	return EOK;
}

/** Post pointing device event to input event queue.
 *
 * @param disp Display
 * @param kbd Keyboard event
 *
 * @return EOK on success or an error code
 */
errno_t ds_ievent_post_ptd(ds_display_t *disp, ptd_event_t *ptd)
{
	ds_ievent_t *ievent;
	ds_ievent_t *prev;
	link_t *link;

	ievent = calloc(1, sizeof(ds_ievent_t));
	if (ievent == NULL)
		return ENOMEM;

	link = list_last(&disp->ievents);
	if (link != NULL) {
		prev = list_get_instance(link, ds_ievent_t, lievents);
		if (prev->etype == det_ptd && prev->ev.ptd.pos_id ==
		    ptd->pos_id) {
			/*
			 * Previous event is also a pointing device event
			 * and it is from the same device.
			 */
			if (prev->ev.ptd.type == PTD_MOVE &&
			    ptd->type == PTD_MOVE) {
				/* Both events are relative move events */
				gfx_coord2_add(&ptd->dmove, &prev->ev.ptd.dmove,
				    &prev->ev.ptd.dmove);
				return EOK;
			} else if (prev->ev.ptd.type == PTD_ABS_MOVE &&
			    ptd->type == PTD_ABS_MOVE) {
				/* Both events are absolute move events */
				prev->ev.ptd.apos = ptd->apos;
				prev->ev.ptd.abounds = ptd->abounds;
				return EOK;
			}
		}
	}

	ievent->display = disp;
	ievent->etype = det_ptd;
	ievent->ev.ptd = *ptd;

	list_append(&ievent->lievents, &disp->ievents);
	fibril_condvar_signal(&disp->ievent_cv);

	return EOK;
}

/** Input event processing fibril.
 *
 * @param arg Argument (ds_display_t *)
 * @return EOK success
 */
static errno_t ds_ievent_fibril(void *arg)
{
	ds_display_t *disp = (ds_display_t *)arg;
	ds_ievent_t *ievent;
	link_t *link;

	fibril_mutex_lock(&disp->lock);

	while (!disp->ievent_quit) {
		while (list_empty(&disp->ievents))
			fibril_condvar_wait(&disp->ievent_cv, &disp->lock);

		link = list_first(&disp->ievents);
		assert(link != NULL);
		list_remove(link);
		ievent = list_get_instance(link, ds_ievent_t, lievents);

		switch (ievent->etype) {
		case det_kbd:
			(void)ds_display_post_kbd_event(disp, &ievent->ev.kbd);
			break;
		case det_ptd:
			(void)ds_display_post_ptd_event(disp, &ievent->ev.ptd);
			break;
		}
	}

	/* Signal to ds_ievent_fini() that the event processing fibril quit */
	disp->ievent_done = true;
	fibril_condvar_signal(&disp->ievent_cv);
	fibril_mutex_unlock(&disp->lock);

	return EOK;
}

/** Initialize input event processing.
 *
 * @param disp Display
 * @return EOK on success or an error code
 */
errno_t ds_ievent_init(ds_display_t *disp)
{
	assert(disp->ievent_fid == 0);

	disp->ievent_fid = fibril_create(ds_ievent_fibril, (void *)disp);
	if (disp->ievent_fid == 0)
		return ENOMEM;

	fibril_add_ready(disp->ievent_fid);
	return EOK;
}

/** Deinitialize input event processing.
 *
 * @param disp Display
 */
void ds_ievent_fini(ds_display_t *disp)
{
	ds_ievent_t *ievent;
	link_t *link;

	if (disp->ievent_fid == 0)
		return;

	/* Signal event processing fibril to quit. */
	fibril_mutex_lock(&disp->lock);
	disp->ievent_quit = true;
	fibril_condvar_signal(&disp->ievent_cv);

	/* Wait for event processing fibril to quit. */
	while (!disp->ievent_done)
		fibril_condvar_wait(&disp->ievent_cv, &disp->lock);

	/* Remove all events from the queue. */
	while (!list_empty(&disp->ievents)) {
		link = list_first(&disp->ievents);
		assert(link != NULL);
		list_remove(link);
		ievent = list_get_instance(link, ds_ievent_t, lievents);
		free(ievent);
	}

	fibril_mutex_unlock(&disp->lock);

	fibril_detach(disp->ievent_fid);
	disp->ievent_fid = 0;
}

/** @}
 */
