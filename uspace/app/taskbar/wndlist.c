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
/** @file Task bar window list
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "clock.h"
#include "wndlist.h"

/** Create task bar window list.
 *
 * @param res UI resource
 * @param fixed Fixed layout to which buttons will be added
 * @param rwndlist Place to store pointer to new window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_create(ui_resource_t *res, ui_fixed_t *fixed,
    wndlist_t **rwndlist)
{
	wndlist_t *wndlist = NULL;
	errno_t rc;

	wndlist = calloc(1, sizeof(wndlist_t));
	if (wndlist == NULL) {
		rc = ENOMEM;
		goto error;
	}

	wndlist->res = res;
	wndlist->fixed = fixed;
	list_initialize(&wndlist->entries);
	*rwndlist = wndlist;
	return EOK;
error:
	return rc;

}

/** Destroy task bar window list. */
void wndlist_destroy(wndlist_t *wndlist)
{
	free(wndlist);
}

/** Append new entry to window list.
 *
 * @param wndlist Window list
 * @param caption Entry caption
 * @return @c EOK on success or an error code
 */
errno_t wndlist_append(wndlist_t *wndlist, const char *caption)
{
	wndlist_entry_t *entry = NULL;
	gfx_rect_t rect;
	errno_t rc;

	entry = calloc(1, sizeof(wndlist_entry_t));
	if (entry == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_pbutton_create(wndlist->res, caption, &entry->button);
	if (rc != EOK)
		goto error;

	if (ui_resource_is_textmode(wndlist->res)) {
		rect.p0.x = 9;
		rect.p0.y = 0;
		rect.p1.x = 25;
		rect.p1.y = 1;
	} else {
		rect.p0.x = 90;
		rect.p0.y = 3;
		rect.p1.x = 230;
		rect.p1.y = 29;
	}

	ui_pbutton_set_rect(entry->button, &rect);

	rc = ui_fixed_add(wndlist->fixed, ui_pbutton_ctl(entry->button));
	if (rc != EOK)
		goto error;

	entry->wndlist = wndlist;
	list_append(&entry->lentries, &wndlist->entries);

	return EOK;
error:
	if (entry != NULL && entry->button != NULL)
		ui_pbutton_destroy(entry->button);
	if (entry != NULL)
		free(entry);
	return rc;

}

/** @}
 */
