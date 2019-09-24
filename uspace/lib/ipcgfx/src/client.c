/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file GFX IPC backend
 *
 * This implements a graphics context via HelenOS IPC.
 */

#include <ipcgfx/client.h>
#include <ipcgfx/ipc/gc.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <stdlib.h>
#include "../private/client.h"

static errno_t ipc_gc_set_color(void *, gfx_color_t *);
static errno_t ipc_gc_fill_rect(void *, gfx_rect_t *);

gfx_context_ops_t ipc_gc_ops = {
	.set_color = ipc_gc_set_color,
	.fill_rect = ipc_gc_fill_rect
};

/** Set color on IPC GC.
 *
 * Set drawing color on IPC GC.
 *
 * @param arg IPC GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_set_color(void *arg, gfx_color_t *color)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	uint16_t r, g, b;
	errno_t rc;

	gfx_color_get_rgb_i16(color, &r, &g, &b);

	exch = async_exchange_begin(ipcgc->sess);
	rc = async_req_3_0(exch, GC_SET_RGB_COLOR, r, g, b);
	async_exchange_end(exch);

	return rc;
}

/** Fill rectangle on IPC GC.
 *
 * @param arg IPC GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(ipcgc->sess);
	rc = async_req_4_0(exch, GC_FILL_RECT, rect->p0.x, rect->p0.y,
	    rect->p1.x, rect->p1.y);
	async_exchange_end(exch);

	return rc;
}

/** Create IPC GC.
 *
 * Create graphics context for rendering via IPC.
 *
 * @param sess Async session
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t ipc_gc_create(async_sess_t *sess, ipc_gc_t **rgc)
{
	ipc_gc_t *ipcgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	ipcgc = calloc(1, sizeof(ipc_gc_t));
	if (ipcgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&ipc_gc_ops, ipcgc, &gc);
	if (rc != EOK)
		goto error;

	ipcgc->gc = gc;
	ipcgc->sess = sess;
	*rgc = ipcgc;
	return EOK;
error:
	if (ipcgc != NULL)
		free(ipcgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete IPC GC.
 *
 * @param ipcgc IPC GC
 */
errno_t ipc_gc_delete(ipc_gc_t *ipcgc)
{
	errno_t rc;

	rc = gfx_context_delete(ipcgc->gc);
	if (rc != EOK)
		return rc;

	free(ipcgc);
	return EOK;
}

/** Get generic graphic context from IPC GC.
 *
 * @param ipcgc IPC GC
 * @return Graphic context
 */
gfx_context_t *ipc_gc_get_ctx(ipc_gc_t *ipcgc)
{
	return ipcgc->gc;
}

/** @}
 */
