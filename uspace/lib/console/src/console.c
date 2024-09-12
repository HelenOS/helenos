/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup libconsole
 * @{
 */
/** @file
 */

#include <as.h>
#include <dbgcon.h>
#include <libc.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/vfs_sess.h>
#include <io/console.h>
#include <ipc/console.h>

console_ctrl_t *console_init(FILE *ifile, FILE *ofile)
{
	console_ctrl_t *ctrl = malloc(sizeof(console_ctrl_t));
	if (!ctrl)
		return NULL;

	ctrl->input_sess = vfs_fsession(ifile, INTERFACE_CONSOLE);
	if (!ctrl->input_sess) {
		free(ctrl);
		return NULL;
	}

	ctrl->output_sess = vfs_fsession(ofile, INTERFACE_CONSOLE);
	if (!ctrl->output_sess) {
		free(ctrl);
		return NULL;
	}

	ctrl->input = ifile;
	ctrl->output = ofile;
	ctrl->input_aid = 0;

	return ctrl;
}

void console_done(console_ctrl_t *ctrl)
{
	free(ctrl);
}

bool console_kcon(void)
{
	return dbgcon_enable();
}

void console_flush(console_ctrl_t *ctrl)
{
	fflush(ctrl->output);
}

void console_clear(console_ctrl_t *ctrl)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_0_0(exch, CONSOLE_CLEAR);
	async_exchange_end(exch);
}

errno_t console_get_size(console_ctrl_t *ctrl, sysarg_t *cols, sysarg_t *rows)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	errno_t rc = async_req_0_2(exch, CONSOLE_GET_SIZE, cols, rows);
	async_exchange_end(exch);

	return rc;
}

void console_set_style(console_ctrl_t *ctrl, uint8_t style)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_1_0(exch, CONSOLE_SET_STYLE, style);
	async_exchange_end(exch);
}

void console_set_color(console_ctrl_t *ctrl, uint8_t bgcolor, uint8_t fgcolor,
    uint8_t flags)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_3_0(exch, CONSOLE_SET_COLOR, bgcolor, fgcolor, flags);
	async_exchange_end(exch);
}

void console_set_rgb_color(console_ctrl_t *ctrl, uint32_t bgcolor,
    uint32_t fgcolor)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_2_0(exch, CONSOLE_SET_RGB_COLOR, bgcolor, fgcolor);
	async_exchange_end(exch);
}

void console_cursor_visibility(console_ctrl_t *ctrl, bool show)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_1_0(exch, CONSOLE_SET_CURSOR_VISIBILITY, (show != false));
	async_exchange_end(exch);
}

/** Set console caption.
 *
 * Set caption text for the console (if the console suports captions).
 *
 * @param ctrl Console
 * @param caption Caption text
 * @return EOK on success or an error code
 */
errno_t console_set_caption(console_ctrl_t *ctrl, const char *caption)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	ipc_call_t answer;
	aid_t req = async_send_0(exch, CONSOLE_SET_CAPTION, &answer);
	errno_t retval = async_data_write_start(exch, caption, str_size(caption));

	if (retval != EOK) {
		async_forget(req);
		async_exchange_end(exch);
		return retval;
	}

	async_wait_for(req, &retval);
	async_exchange_end(exch);
	return EOK;
}

errno_t console_get_color_cap(console_ctrl_t *ctrl, sysarg_t *ccap)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	errno_t rc = async_req_0_1(exch, CONSOLE_GET_COLOR_CAP, ccap);
	async_exchange_end(exch);

	return rc;
}

errno_t console_get_pos(console_ctrl_t *ctrl, sysarg_t *col, sysarg_t *row)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	errno_t rc = async_req_0_2(exch, CONSOLE_GET_POS, col, row);
	async_exchange_end(exch);

	return rc;
}

void console_set_pos(console_ctrl_t *ctrl, sysarg_t col, sysarg_t row)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	async_req_2_0(exch, CONSOLE_SET_POS, col, row);
	async_exchange_end(exch);
}

static errno_t console_ev_decode(ipc_call_t *call, cons_event_t *event)
{
	event->type = ipc_get_arg1(call);

	switch (event->type) {
	case CEV_KEY:
		event->ev.key.type = ipc_get_arg2(call);
		event->ev.key.key = ipc_get_arg3(call);
		event->ev.key.mods = ipc_get_arg4(call);
		event->ev.key.c = ipc_get_arg5(call);
		return EOK;
	case CEV_POS:
		event->ev.pos.pos_id = ipc_get_arg2(call) >> 16;
		event->ev.pos.type = ipc_get_arg2(call) & 0xffff;
		event->ev.pos.btn_num = ipc_get_arg3(call);
		event->ev.pos.hpos = ipc_get_arg4(call);
		event->ev.pos.vpos = ipc_get_arg5(call);
		return EOK;
	case CEV_RESIZE:
		return EOK;
	}

	return EIO;
}

/** Get console event.
 *
 * @param ctrl Console
 * @param event Place to store event
 * @return EOK on success, EIO on failure
 */
errno_t console_get_event(console_ctrl_t *ctrl, cons_event_t *event)
{
	if (ctrl->input_aid == 0) {
		ipc_call_t result;

		async_exch_t *exch = async_exchange_begin(ctrl->input_sess);
		aid_t aid = async_send_0(exch, CONSOLE_GET_EVENT, &result);
		async_exchange_end(exch);

		errno_t rc;
		async_wait_for(aid, &rc);

		if (rc != EOK)
			return EIO;

		rc = console_ev_decode(&result, event);
		if (rc != EOK)
			return EIO;
	} else {
		errno_t retval;
		async_wait_for(ctrl->input_aid, &retval);

		ctrl->input_aid = 0;

		if (retval != EOK)
			return EIO;

		errno_t rc = console_ev_decode(&ctrl->input_call, event);
		if (rc != EOK)
			return EIO;
	}

	return EOK;
}

/** Get console event with timeout.
 *
 * @param ctrl Console
 * @param event Place to store event
 * @param timeout Pointer to timeout. This will be updated to reflect
 *                the remaining time in case of timeout.
 * @return EOK on success (event received), ETIMEOUT on time out,
 *         EIO on I/O error (e.g. lost console connection), ENOMEM
 *         if out of memory
 */
errno_t console_get_event_timeout(console_ctrl_t *ctrl, cons_event_t *event,
    usec_t *timeout)
{
	struct timespec t0;
	getuptime(&t0);

	if (ctrl->input_aid == 0) {
		async_exch_t *exch = async_exchange_begin(ctrl->input_sess);
		ctrl->input_aid = async_send_0(exch, CONSOLE_GET_EVENT,
		    &ctrl->input_call);
		async_exchange_end(exch);
	}

	errno_t retval;
	errno_t rc = async_wait_timeout(ctrl->input_aid, &retval, *timeout);
	if (rc != EOK) {
		if (rc == ENOMEM)
			return ENOMEM;
		*timeout = 0;
		return ETIMEOUT;
	}

	ctrl->input_aid = 0;

	if (retval != EOK)
		return EIO;

	rc = console_ev_decode(&ctrl->input_call, event);
	if (rc != EOK)
		return EIO;

	/* Update timeout */
	struct timespec t1;
	getuptime(&t1);
	*timeout -= NSEC2USEC(ts_sub_diff(&t1, &t0));

	return EOK;
}

/** Create a shared buffer for fast rendering to the console.
 *
 * @param ctrl Console
 * @param cols Number of columns
 * @param rows Number of rows
 * @param rbuf Place to store pointer to the shared buffer
 * @return EOK on success or an error code
 */
errno_t console_map(console_ctrl_t *ctrl, sysarg_t cols, sysarg_t rows,
    charfield_t **rbuf)
{
	async_exch_t *exch = NULL;
	void *buf;
	aid_t req;
	ipc_call_t answer;
	size_t asize;
	errno_t rc;

	exch = async_exchange_begin(ctrl->output_sess);
	req = async_send_2(exch, CONSOLE_MAP, cols, rows, &answer);

	asize = PAGES2SIZE(SIZE2PAGES(cols * rows * sizeof(charfield_t)));

	rc = async_share_in_start_0_0(exch, asize, &buf);
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}

	async_exchange_end(exch);
	exch = NULL;

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	*rbuf = (charfield_t *)buf;
	return EOK;
error:
	if (exch != NULL)
		async_exchange_end(exch);
	return rc;
}

/** Unmap console shared buffer.
 *
 * @param ctrl Console
 * @param buf Buffer
 */
void console_unmap(console_ctrl_t *ctrl, charfield_t *buf)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	(void) async_req_0_0(exch, CONSOLE_UNMAP);
	async_exchange_end(exch);

	as_area_destroy(buf);
}

/** Update console rectangle from shared buffer.
 *
 * @param ctrl Console
 * @param c0 Column coordinate of top-left corner (inclusive)
 * @param r0 Row coordinate of top-left corner (inclusive)
 * @param c1 Column coordinate of bottom-right corner (exclusive)
 * @param r1 Row coordinate of bottom-right corner (exclusive)
 *
 * @return EOK on sucess or an error code
 */
errno_t console_update(console_ctrl_t *ctrl, sysarg_t c0, sysarg_t r0,
    sysarg_t c1, sysarg_t r1)
{
	async_exch_t *exch = async_exchange_begin(ctrl->output_sess);
	errno_t rc = async_req_4_0(exch, CONSOLE_UPDATE, c0, r0, c1, r1);
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
