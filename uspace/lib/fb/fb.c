/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <as.h>
#include "fb.h"

static async_exch_t *vp_exchange_begin(async_sess_t *sess, vp_handle_t vp)
{
	vp_handle_t cur_vp = (vp_handle_t) async_remote_state_acquire(sess);
	async_exch_t *exch = async_exchange_begin(sess);
	
	if (cur_vp != vp) {
		int ret = async_req_1_0(exch, FB_VP_FOCUS, vp);
		if (ret != EOK) {
			async_exchange_end(exch);
			return NULL;
		}
		
		async_remote_state_update(sess, (void *) vp);
	}
	
	return exch;
}

static void vp_exchange_end(async_exch_t *exch)
{
	async_remote_state_release_exchange(exch);
}

int fb_get_resolution(async_sess_t *sess, sysarg_t *maxx, sysarg_t *maxy)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_0_2(exch, FB_GET_RESOLUTION, maxx, maxy);
	async_exchange_end(exch);
	
	return ret;
}

int fb_yield(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_0_0(exch, FB_YIELD);
	async_exchange_end(exch);
	
	return ret;
}

int fb_claim(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_0_0(exch, FB_CLAIM);
	async_exchange_end(exch);
	
	return ret;
}

int fb_pointer_update(async_sess_t *sess, sysarg_t x, sysarg_t y, bool visible)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_3_0(exch, FB_POINTER_UPDATE, x, y, visible);
	async_exchange_end(exch);
	
	return ret;
}

vp_handle_t fb_vp_create(async_sess_t *sess, sysarg_t x, sysarg_t y,
    sysarg_t width, sysarg_t height)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	vp_handle_t handle;
	int ret = async_req_4_1(exch, FB_VP_CREATE, x, y, width, height,
	    &handle);
	
	async_exchange_end(exch);
	
	if (ret != EOK)
		return 0;
	
	return handle;
}

frontbuf_handle_t fb_frontbuf_create(async_sess_t *sess,
    screenbuffer_t *frontbuf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, FB_FRONTBUF_CREATE, &answer);
	int rc = async_share_out_start(exch, frontbuf, AS_AREA_READ
	    | AS_AREA_WRITE | AS_AREA_CACHEABLE);
	
	async_exchange_end(exch);
	
	sysarg_t ret;
	async_wait_for(req, &ret);
	
	if ((rc != EOK) || (ret != EOK))
		return 0;
	
	return (frontbuf_handle_t) IPC_GET_ARG1(answer);
}

imagemap_handle_t fb_imagemap_create(async_sess_t *sess, imgmap_t *imgmap)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, FB_IMAGEMAP_CREATE, &answer);
	int rc = async_share_out_start(exch, imgmap, AS_AREA_READ
	    | AS_AREA_WRITE | AS_AREA_CACHEABLE);
	
	async_exchange_end(exch);
	
	sysarg_t ret;
	async_wait_for(req, &ret);
	
	if ((rc != EOK) || (ret != EOK))
		return 0;
	
	return (imagemap_handle_t) IPC_GET_ARG1(answer);
}

sequence_handle_t fb_sequence_create(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	
	sysarg_t ret;
	int rc = async_req_0_1(exch, FB_SEQUENCE_CREATE, &ret);
	
	async_exchange_end(exch);
	
	if (rc != EOK)
		return 0;
	
	return (sequence_handle_t) ret;
}

int fb_vp_get_dimensions(async_sess_t *sess, vp_handle_t vp, sysarg_t *cols,
    sysarg_t *rows)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_0_2(exch, FB_VP_GET_DIMENSIONS, cols, rows);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_get_caps(async_sess_t *sess, vp_handle_t vp, console_caps_t *ccaps)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	
	sysarg_t rv;
	int ret = async_req_0_1(exch, FB_VP_GET_CAPS, &rv);
	
	vp_exchange_end(exch);
	
	if (ret == EOK)
		*ccaps = (console_caps_t) rv;
	
	return ret;
}

int fb_vp_clear(async_sess_t *sess, vp_handle_t vp)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_0_0(exch, FB_VP_CLEAR);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_cursor_update(async_sess_t *sess, vp_handle_t vp,
    frontbuf_handle_t frontbuf)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_1_0(exch, FB_VP_CURSOR_UPDATE, frontbuf);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_set_style(async_sess_t *sess, vp_handle_t vp, console_style_t style)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_1_0(exch, FB_VP_SET_STYLE, style);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_putchar(async_sess_t *sess, vp_handle_t vp, sysarg_t col,
    sysarg_t row, wchar_t ch)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_3_0(exch, FB_VP_PUTCHAR, col, row, ch);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_update(async_sess_t *sess, vp_handle_t vp, frontbuf_handle_t frontbuf)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_1_0(exch, FB_VP_UPDATE, frontbuf);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_damage(async_sess_t *sess, vp_handle_t vp, frontbuf_handle_t frontbuf,
    sysarg_t col, sysarg_t row, sysarg_t cols, sysarg_t rows)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_5_0(exch, FB_VP_DAMAGE, frontbuf, col, row,
	    cols, rows);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_vp_imagemap_damage(async_sess_t *sess, vp_handle_t vp,
    imagemap_handle_t imagemap, sysarg_t x, sysarg_t y, sysarg_t width,
    sysarg_t height)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_5_0(exch, FB_VP_IMAGEMAP_DAMAGE, imagemap, x, y,
	    width, height);
	vp_exchange_end(exch);
	
	return ret;
}

int fb_sequence_add_imagemap(async_sess_t *sess, sequence_handle_t sequence,
    imagemap_handle_t imagemap)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_2_0(exch, FB_SEQUENCE_ADD_IMAGEMAP, sequence,
	    imagemap);
	async_exchange_end(exch);
	
	return ret;
}

int fb_vp_sequence_start(async_sess_t *sess, vp_handle_t vp,
    sequence_handle_t sequence)
{
	async_exch_t *exch = vp_exchange_begin(sess, vp);
	int ret = async_req_1_0(exch, FB_VP_SEQUENCE_START, sequence);
	vp_exchange_end(exch);
	
	return ret;
}

/** @}
 */
