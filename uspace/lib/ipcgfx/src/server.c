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
 * @file GFX IPC server
 *
 * Serve a graphics context via HelenOS IPC.
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <ipc/bd.h>
#include <ipcgfx/ipc/gc.h>
#include <ipcgfx/server.h>
#include <stdint.h>

#include <bd_srv.h>

static void gc_set_rgb_color_srv(gfx_context_t *gc, ipc_call_t *call)
{
	uint16_t r, g, b;
	gfx_color_t *color;
	errno_t rc;

	r = ipc_get_arg1(call);
	g = ipc_get_arg2(call);
	b = ipc_get_arg3(call);

	rc = gfx_color_new_rgb_i16(r, g, b, &color);
	if (rc != EOK) {
		async_answer_0(call, ENOMEM);
		return;
	}

	rc = gfx_set_color(gc, color);
	async_answer_0(call, rc);
}

static void gc_fill_rect_srv(gfx_context_t *gc, ipc_call_t *call)
{
	gfx_rect_t rect;
	errno_t rc;

	rect.p0.x = ipc_get_arg1(call);
	rect.p0.y = ipc_get_arg2(call);
	rect.p1.x = ipc_get_arg3(call);
	rect.p1.y = ipc_get_arg4(call);

	rc = gfx_fill_rect(gc, &rect);
	async_answer_0(call, rc);
}

errno_t gc_conn(ipc_call_t *icall, gfx_context_t *gc)
{
	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case GC_SET_RGB_COLOR:
			gc_set_rgb_color_srv(gc, &call);
			break;
		case GC_FILL_RECT:
			gc_fill_rect_srv(gc, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}

	return EOK;
}

/** @}
 */
