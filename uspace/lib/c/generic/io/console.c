/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2006 Jakub Vana
 * Copyright (c) 2008 Jiri Svoboda
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

#include <libc.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
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
	return __SYSCALL0(SYS_DEBUG_CONSOLE);
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
	event->type = IPC_GET_ARG1(*call);

	switch (event->type) {
	case CEV_KEY:
		event->ev.key.type = IPC_GET_ARG2(*call);
		event->ev.key.key = IPC_GET_ARG3(*call);
		event->ev.key.mods = IPC_GET_ARG4(*call);
		event->ev.key.c = IPC_GET_ARG5(*call);
		break;
	case CEV_POS:
		event->ev.pos.pos_id = IPC_GET_ARG2(*call) >> 16;
		event->ev.pos.type = IPC_GET_ARG2(*call) & 0xffff;
		event->ev.pos.btn_num = IPC_GET_ARG3(*call);
		event->ev.pos.hpos = IPC_GET_ARG4(*call);
		event->ev.pos.vpos = IPC_GET_ARG5(*call);
		break;
	default:
		return EIO;
	}

	return EOK;
}

bool console_get_event(console_ctrl_t *ctrl, cons_event_t *event)
{
	if (ctrl->input_aid == 0) {
		ipc_call_t result;

		async_exch_t *exch = async_exchange_begin(ctrl->input_sess);
		aid_t aid = async_send_0(exch, CONSOLE_GET_EVENT, &result);
		async_exchange_end(exch);

		errno_t rc;
		async_wait_for(aid, &rc);

		if (rc != EOK) {
			errno = rc;
			return false;
		}

		rc = console_ev_decode(&result, event);
		if (rc != EOK) {
			errno = rc;
			return false;
		}
	} else {
		errno_t retval;
		async_wait_for(ctrl->input_aid, &retval);

		ctrl->input_aid = 0;

		if (retval != EOK) {
			errno = retval;
			return false;
		}

		errno_t rc = console_ev_decode(&ctrl->input_call, event);
		if (rc != EOK) {
			errno = rc;
			return false;
		}
	}

	return true;
}

bool console_get_event_timeout(console_ctrl_t *ctrl, cons_event_t *event,
    suseconds_t *timeout)
{
	struct timeval t0;
	gettimeofday(&t0, NULL);

	if (ctrl->input_aid == 0) {
		async_exch_t *exch = async_exchange_begin(ctrl->input_sess);
		ctrl->input_aid = async_send_0(exch, CONSOLE_GET_EVENT,
		    &ctrl->input_call);
		async_exchange_end(exch);
	}

	errno_t retval;
	errno_t rc = async_wait_timeout(ctrl->input_aid, &retval, *timeout);
	if (rc != EOK) {
		*timeout = 0;
		errno = rc;
		return false;
	}

	ctrl->input_aid = 0;

	if (retval != EOK) {
		errno = retval;
		return false;
	}

	rc = console_ev_decode(&ctrl->input_call, event);
	if (rc != EOK) {
		errno = rc;
		return false;
	}

	/* Update timeout */
	struct timeval t1;
	gettimeofday(&t1, NULL);
	*timeout -= tv_sub_diff(&t1, &t0);

	return true;
}

/** @}
 */
