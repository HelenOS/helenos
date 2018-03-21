/*
 * Copyright (c) 2012 Jiri Svoboda
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
/**
 * @file
 * @brief Input protocol client stub
 */

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <io/kbd_event.h>
#include <io/input.h>
#include <ipc/input.h>
#include <stdlib.h>

static void input_cb_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg);

errno_t input_open(async_sess_t *sess, input_ev_ops_t *ev_ops,
    void *arg, input_t **rinput)
{
	input_t *input = calloc(1, sizeof(input_t));
	if (input == NULL)
		return ENOMEM;

	input->sess = sess;
	input->ev_ops = ev_ops;
	input->user = arg;

	async_exch_t *exch = async_exchange_begin(sess);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_INPUT_CB, 0, 0,
	    input_cb_conn, input, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		goto error;

	*rinput = input;
	return EOK;

error:
	if (input != NULL)
		free(input);

	return rc;
}

void input_close(input_t *input)
{
	/* XXX Synchronize with input_cb_conn */
	free(input);
}

errno_t input_activate(input_t *input)
{
	async_exch_t *exch = async_exchange_begin(input->sess);
	errno_t rc = async_req_0_0(exch, INPUT_ACTIVATE);
	async_exchange_end(exch);

	return rc;
}

static void input_ev_active(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	errno_t rc = input->ev_ops->active(input);
	async_answer_0(chandle, rc);
}

static void input_ev_deactive(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	errno_t rc = input->ev_ops->deactive(input);
	async_answer_0(chandle, rc);
}

static void input_ev_key(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	kbd_event_type_t type;
	keycode_t key;
	keymod_t mods;
	wchar_t c;
	errno_t rc;

	type = IPC_GET_ARG1(*call);
	key = IPC_GET_ARG2(*call);
	mods = IPC_GET_ARG3(*call);
	c = IPC_GET_ARG4(*call);

	rc = input->ev_ops->key(input, type, key, mods, c);
	async_answer_0(chandle, rc);
}

static void input_ev_move(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	int dx;
	int dy;
	errno_t rc;

	dx = IPC_GET_ARG1(*call);
	dy = IPC_GET_ARG2(*call);

	rc = input->ev_ops->move(input, dx, dy);
	async_answer_0(chandle, rc);
}

static void input_ev_abs_move(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	unsigned x;
	unsigned y;
	unsigned max_x;
	unsigned max_y;
	errno_t rc;

	x = IPC_GET_ARG1(*call);
	y = IPC_GET_ARG2(*call);
	max_x = IPC_GET_ARG3(*call);
	max_y = IPC_GET_ARG4(*call);

	rc = input->ev_ops->abs_move(input, x, y, max_x, max_y);
	async_answer_0(chandle, rc);
}

static void input_ev_button(input_t *input, cap_call_handle_t chandle,
    ipc_call_t *call)
{
	int bnum;
	int press;
	errno_t rc;

	bnum = IPC_GET_ARG1(*call);
	press = IPC_GET_ARG2(*call);

	rc = input->ev_ops->button(input, bnum, press);
	async_answer_0(chandle, rc);
}

static void input_cb_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	input_t *input = (input_t *)arg;

	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case INPUT_EVENT_ACTIVE:
			input_ev_active(input, chandle, &call);
			break;
		case INPUT_EVENT_DEACTIVE:
			input_ev_deactive(input, chandle, &call);
			break;
		case INPUT_EVENT_KEY:
			input_ev_key(input, chandle, &call);
			break;
		case INPUT_EVENT_MOVE:
			input_ev_move(input, chandle, &call);
			break;
		case INPUT_EVENT_ABS_MOVE:
			input_ev_abs_move(input, chandle, &call);
			break;
		case INPUT_EVENT_BUTTON:
			input_ev_button(input, chandle, &call);
			break;
		default:
			async_answer_0(chandle, ENOTSUP);
		}
	}
}

/** @}
 */
