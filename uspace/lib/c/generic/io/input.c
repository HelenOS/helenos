/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void input_cb_conn(ipc_call_t *icall, void *arg);

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

static void input_ev_active(input_t *input, ipc_call_t *call)
{
	errno_t rc = input->ev_ops->active(input);
	async_answer_0(call, rc);
}

static void input_ev_deactive(input_t *input, ipc_call_t *call)
{
	errno_t rc = input->ev_ops->deactive(input);
	async_answer_0(call, rc);
}

static void input_ev_key(input_t *input, ipc_call_t *call)
{
	kbd_event_type_t type;
	keycode_t key;
	keymod_t mods;
	char32_t c;
	errno_t rc;

	type = ipc_get_arg1(call);
	key = ipc_get_arg2(call);
	mods = ipc_get_arg3(call);
	c = ipc_get_arg4(call);

	rc = input->ev_ops->key(input, type, key, mods, c);
	async_answer_0(call, rc);
}

static void input_ev_move(input_t *input, ipc_call_t *call)
{
	int dx;
	int dy;
	errno_t rc;

	dx = ipc_get_arg1(call);
	dy = ipc_get_arg2(call);

	rc = input->ev_ops->move(input, dx, dy);
	async_answer_0(call, rc);
}

static void input_ev_abs_move(input_t *input, ipc_call_t *call)
{
	unsigned x;
	unsigned y;
	unsigned max_x;
	unsigned max_y;
	errno_t rc;

	x = ipc_get_arg1(call);
	y = ipc_get_arg2(call);
	max_x = ipc_get_arg3(call);
	max_y = ipc_get_arg4(call);

	rc = input->ev_ops->abs_move(input, x, y, max_x, max_y);
	async_answer_0(call, rc);
}

static void input_ev_button(input_t *input, ipc_call_t *call)
{
	int bnum;
	int press;
	errno_t rc;

	bnum = ipc_get_arg1(call);
	press = ipc_get_arg2(call);

	rc = input->ev_ops->button(input, bnum, press);
	async_answer_0(call, rc);
}

static void input_ev_dclick(input_t *input, ipc_call_t *call)
{
	int bnum;
	errno_t rc;

	bnum = ipc_get_arg1(call);

	rc = input->ev_ops->dclick(input, bnum);
	async_answer_0(call, rc);
}

static void input_cb_conn(ipc_call_t *icall, void *arg)
{
	input_t *input = (input_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case INPUT_EVENT_ACTIVE:
			input_ev_active(input, &call);
			break;
		case INPUT_EVENT_DEACTIVE:
			input_ev_deactive(input, &call);
			break;
		case INPUT_EVENT_KEY:
			input_ev_key(input, &call);
			break;
		case INPUT_EVENT_MOVE:
			input_ev_move(input, &call);
			break;
		case INPUT_EVENT_ABS_MOVE:
			input_ev_abs_move(input, &call);
			break;
		case INPUT_EVENT_BUTTON:
			input_ev_button(input, &call);
			break;
		case INPUT_EVENT_DCLICK:
			input_ev_dclick(input, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}
}

/** @}
 */
