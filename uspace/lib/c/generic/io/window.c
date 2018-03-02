/*
 * Copyright (c) 2012 Petr Koupy
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

#include <errno.h>
#include <as.h>
#include <ipc/window.h>
#include <io/window.h>

#include <stdio.h>

errno_t win_register(async_sess_t *sess, window_flags_t flags, service_id_t *in,
    service_id_t *out)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_1_2(exch, WINDOW_REGISTER, flags, in, out);
	async_exchange_end(exch);

	return ret;
}

errno_t win_get_event(async_sess_t *sess, window_event_t *event)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, WINDOW_GET_EVENT, &answer);

	errno_t rc = async_data_read_start(exch, event, sizeof(window_event_t));

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t win_damage(async_sess_t *sess,
    sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_4_0(exch, WINDOW_DAMAGE, x, y, width, height);
	async_exchange_end(exch);

	return ret;
}

errno_t win_grab(async_sess_t *sess, sysarg_t pos_id, sysarg_t grab_flags)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_2_0(exch, WINDOW_GRAB, pos_id, grab_flags);
	async_exchange_end(exch);

	return ret;
}

errno_t win_resize(async_sess_t *sess, sysarg_t x, sysarg_t y, sysarg_t width,
    sysarg_t height, window_placement_flags_t placement_flags, void *cells)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_5(exch, WINDOW_RESIZE, x, y, width, height,
	    (sysarg_t) placement_flags, &answer);

	errno_t rc = async_share_out_start(exch, cells, AS_AREA_READ | AS_AREA_CACHEABLE);

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK)
		return rc;
	else if (ret != EOK)
		return ret;

	return EOK;
}

errno_t win_close(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, WINDOW_CLOSE);
	async_exchange_end(exch);

	return ret;
}

errno_t win_close_request(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, WINDOW_CLOSE_REQUEST);
	async_exchange_end(exch);

	return ret;
}

/** @}
 */
